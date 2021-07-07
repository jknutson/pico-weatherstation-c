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
	"syscall"

	MQTT "github.com/eclipse/paho.mqtt.golang"
	"github.com/tarm/serial"
)

// TODO: default serial port based on OS
var (
	//BuildVersion is passed at build: `-X main.BuildVersion=0.1.0`
	BuildVersion                  string
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

// TODO: make generic message type,
//       update pico to emit data type as field

// DHTMessage represents JSON message of DHT11/22 sensor data
type DHTMessage struct {
	Humidity     float64 `json:"humidity"`
	TemperatureF float64 `json:"temperature_f"`
}

// WindAngleMessage represents JSON message of wind angle in degrees (0 = N)
type WindAngleMessage struct {
	WindAngle float64 `json:"wind_angle"`
}

//WindSpeedMessage represnets JSON message of wind speed in mph
type WindSpeedMessage struct {
	WindSpeed float64 `json:"wind_speed"`
}

func usage() {
	println(`Usage: pico-weather-reader [options]
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

	log.Printf("reading from %s\n", serialPort)
	for {
		reader := bufio.NewReader(s)
		line, err := reader.ReadString('\n')
		if err != nil && err != io.EOF {
			panic(err)
		}
		if line != "" {
			if verbose {
				log.Printf("line: %s", line)
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
				token := c.Publish(fmt.Sprintf("%s/dht/humidity", mqTopic), 0, false, dhtMatches[0][1])
				token.Wait()
				if token.Error() != nil {
					log.Printf("mq publish error: %s\n", token.Error())
				}
				token = c.Publish(fmt.Sprintf("%s/dht/temperature", mqTopic), 0, false, dhtMatches[0][2])
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
				log.Printf("other: %s", line)
			}

			dhtMatched = false
			windAngleMatched = false
			windSpeedMatched = false
		}
	}
}
