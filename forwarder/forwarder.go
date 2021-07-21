package main

import (
	"bufio"
	"flag"
	"fmt"
	"io"
	"log"
	"os"
	"os/signal"
	"regexp"
	"strconv"
	"syscall"
	"time"

	MQTT "github.com/eclipse/paho.mqtt.golang"
	"github.com/tarm/serial"
)

// TODO: default serial port based on OS
var (
	//BuildVersion is passed at build: `-X main.BuildVersion=0.1.0`
	BuildVersion                  string
	samplePeriod                  string = "10s"
	serialPort                    string
	serialPortDefault             string = "/dev/ttyACM0"
	serialBaud                    int
	serialBaudDefault             int = 115200
	mqBroker, mqTopic, mqClientID string
	mqBrokerDefault               string = "tcp://localhost:1883"
	mqTopicDefault                string = "pico-weatherstation"
	mqClientIDDefault             string = "pico-weatherstation"
	verbose, version              bool
)

// Sample...
type Sample struct {
	Points []float64
}

func (s Sample) Mean() float64 {
	var sum float64 = 0
	for i := 0; i < len(s.Points); i++ {
		sum += s.Points[i]
	}
	return sum / float64(len(s.Points))
}

func (s Sample) MeanS() string {
	return fmt.Sprintf("%.2f", s.Mean())
}

func (s Sample) AddPointS(p string) error {
	point_f, err := strconv.ParseFloat(p, 64)
	if err != nil {
		return err
	}
	s.Points = append(s.Points, point_f)
	return nil
}

func usage() {
	println(`Usage: forwarder [options]
	Read data from Serial port and emit to MQTT
	Options:`)
	flag.PrintDefaults()
	println(`For more information, see https://github.com/jknutson/pico-weatherstation-c`)
}

func initFlags() {
	flag.StringVar(&serialPort, "serialPort", serialPortDefault, "path to serial port")
	flag.IntVar(&serialBaud, "serialBaud", serialBaudDefault, "baud rate")
	flag.StringVar(&mqBroker, "mqBroker", mqBrokerDefault, "MQ Broker (Host)")
	flag.StringVar(&mqClientID, "mqClientID", mqClientIDDefault, "MQ Client ID")
	flag.StringVar(&mqTopic, "mqTopic", mqTopicDefault, "MQ Topic Base")
	flag.BoolVar(&verbose, "verbose", false, "verbose output")
	flag.BoolVar(&version, "version", false, "show version")

	flag.Usage = usage
	flag.Parse()

	if version {
		log.Printf("version: %s\n", BuildVersion)
		return
	}
}

func setupCloseHandler() {
	c := make(chan os.Signal)
	signal.Notify(c, os.Interrupt, syscall.SIGTERM)
	go func() {
		<-c
		log.Println("Ctrl+C pressed, exiting.")
		os.Exit(0)
	}()
}

func main() {
	initFlags()
	setupCloseHandler()

	// setup serial port
	ser := &serial.Config{Name: serialPort, Baud: serialBaud}
	s, err := serial.OpenPort(ser)
	if err != nil {
		log.Fatal(err)
	}

	//regexp parsers
	dhtRe := regexp.MustCompile(`.*\"humidity\"\:\s*(\d+\.\d+)\,\s*\"temperature_f\"\:\s*(\d+\.\d+).*`)
	windAngleRe := regexp.MustCompile(`.*\"wind_angle\"\:\s*(\d+\.\d+).*`)
	windSpeedRe := regexp.MustCompile(`.*\"wind_speed\"\:\s*(\d+\.\d+).*`)

	// setup MQTT client
	mqOpts := MQTT.NewClientOptions().AddBroker(mqBroker)
	mqOpts.SetClientID(mqClientID)
	c := MQTT.NewClient(mqOpts)
	if token := c.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	}
	// topics
	dhtHumidityTopic := fmt.Sprintf("iot/%s/humidity", mqTopic)
	dhtTemperatureTopic := fmt.Sprintf("iot/%s/temperature", mqTopic)

	// setup stats/samples
	var dhtHumiditySample Sample
	var dhtTemperatureSample Sample
	start := time.Now()
	sampleDuration, err := time.ParseDuration(samplePeriod)
	if err != nil {
		panic(err)
	}

	log.Printf("reading from %s\n", serialPort)
	for {
		// TODO: try moving reader outside for loop
		reader := bufio.NewReader(s)
		line, err := reader.ReadString('\n')
		if err != nil && err != io.EOF {
			panic(err)
		}
		if line != "" {
			if verbose {
				fmt.Println(strconv.Quote(line))
			}
			dhtMatched, err := regexp.MatchString(`.*\"humidity\"\:.*`, line)
			if err != nil {
				panic(err)
			}
			windAngleMatched, err := regexp.MatchString(`.*\"wind_angle\".*`, line)
			if err != nil {
				panic(err)
			}
			windSpeedMatched, err := regexp.MatchString(`.*\"wind_speed\".*`, line)
			if err != nil {
				panic(err)
			}

			if dhtMatched {
				if verbose {
					log.Printf("DHT: %s", line)
				}
				dhtMatches := dhtRe.FindAllStringSubmatch(line, -1)
				err = dhtHumiditySample.AddPointS(dhtMatches[0][1])
				if err != nil {
					panic(err) // TODO: handle this better
				}
				log.Printf("publishing %s %s\n", dhtHumidityTopic, dhtMatches[0][1])
				token := c.Publish(dhtHumidityTopic, 0, false, dhtMatches[0][1])
				token.Wait()
				if token.Error() != nil {
					log.Printf("mq publish error: %s\n", token.Error())
				}
				log.Printf("%q\n", dhtTemperatureSample)
				err = dhtTemperatureSample.AddPointS(dhtMatches[0][2])
				if err != nil {
					panic(err) // TODO: handle this better
				}
				log.Printf("publishing %s %s\n", dhtTemperatureTopic, dhtMatches[0][2])
				token = c.Publish(dhtTemperatureTopic, 0, false, dhtMatches[0][2])
				token.Wait()
				if token.Error() != nil {
					log.Printf("mq publish error: %s\n", token.Error())
				}
			}
			if windAngleMatched {
				if verbose {
					log.Printf("Wind Angle: %s", line)
				}
				windAngleMatches := windAngleRe.FindAllStringSubmatch(line, -1)
				token := c.Publish(fmt.Sprintf("%s/dht/wind_angle", mqTopic), 0, false, windAngleMatches[0][1])
				token.Wait()
				if token.Error() != nil {
					log.Printf("mq publish error: %s\n", token.Error())
				}
			}
			if windSpeedMatched {
				if verbose {
					log.Printf("Wind Speed: %s", line)
				}
				windSpeedMatches := windSpeedRe.FindAllStringSubmatch(line, -1)
				token := c.Publish(fmt.Sprintf("%s/dht/wind_speed", mqTopic), 0, false, windSpeedMatches[0][1])
				token.Wait()
				if token.Error() != nil {
					log.Printf("mq publish error: %s\n", token.Error())
				}
			}

			if !(dhtMatched || windAngleMatched || windSpeedMatched) {
				if verbose {
					log.Printf("other: %q", line)
				}
			}

			dhtMatched = false
			windAngleMatched = false
			windSpeedMatched = false
		}

		if time.Since(start) >= sampleDuration {
			dhtHumidityAvgTopic := fmt.Sprintf("%s_avg", dhtHumidityTopic)
			// TODO: bug here, the MeanS returns NaN, slice of Points is empty
			dhtHumidityAvgPayload := dhtHumiditySample.MeanS()
			log.Printf("publishing %s %s\n", dhtHumidityAvgTopic, dhtHumidityAvgPayload)
			token := c.Publish(dhtHumidityAvgTopic, 0, false, dhtHumidityAvgPayload)
			token.Wait()
			if token.Error() != nil {
				log.Printf("mq publish error: %s\n", token.Error())
			}
			dhtTemperatureAvgTopic := fmt.Sprintf("%s_avg", dhtTemperatureTopic)
			dhtTemperatureAvgPayload := dhtTemperatureSample.MeanS()
			log.Printf("publishing %s %s\n", dhtTemperatureAvgTopic, dhtTemperatureAvgPayload)
			token = c.Publish(dhtTemperatureAvgTopic, 0, false, dhtTemperatureAvgPayload)
			token.Wait()
			if token.Error() != nil {
				log.Printf("mq publish error: %s\n", token.Error())
			}

			start = time.Now()
		}
	}
}
