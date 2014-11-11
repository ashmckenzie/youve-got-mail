package main

import (
  "time"
  "strings"
  "strconv"
  "os"
  "fmt"
  "bytes"
  "io/ioutil"
  "net/http"

  "menteslibres.net/gosexy/redis"
  "github.com/kr/pretty"
)

var redisHost = os.Getenv("REDIS_HOST")
var redisPort = uint(6379)

var pushBulletURL = "https://api.pushbullet.com/v2/pushes"
var pushBulletToken = os.Getenv("PUSHBULLET_TOKEN")

var redisClient *redis.Client

type Notification struct {
  Key     string    `json:"key"`
  Value   string    `json:"value"`
  Epoch   int64     `json:"epoch"`
}

type RedisChannel struct {
  Base  string
  Name  string
}

func newConsumer() *redis.Client {
  consumer := redis.New()
  consumer.ConnectNonBlock(redisHost, redisPort)

  return consumer
}

func listenToPublish(redisConsumer *redis.Client, rec *chan []string, channel string) {
  go redisConsumer.PSubscribe(*rec, channel)
}

func debugPrint(value interface{}) {
  if os.Getenv("DEBUG") == "true" { pretty.Printf("%#v\n", value) }
}

func (rc RedisChannel) channel() string {
  return rc.Base + ":" + rc.Name
}

type RedisPubSub struct {
  RedisChannel RedisChannel
}

func notificationFromMessage(channelBase string, msg []string) *Notification {
  key     := strings.Replace(msg[2], channelBase + ":", "", 1)
  value   := msg[3]
  epoch   := int64(time.Now().Unix())

  notification := Notification{ key, value, epoch }

  return &notification
}

func (notification Notification) sentToday() bool {
  twelve_hours := int64(12 * (60 * 60))

  s, err := redisClient.Get("youve-got-mail")
  if err != nil { fmt.Println(err) }

  if (len(s) == 0) {
    return false
  }

  i, err := strconv.ParseInt(s, 0, 64)
  if err != nil { fmt.Println(err) }

  now := int64(time.Now().Unix())

  if (now - i > twelve_hours) {
    return false
  } else {
    return true
  }
}

func (notification Notification) setSentToday() {
  s, err := redisClient.Set("youve-got-mail", notification.Epoch)

  if err != nil { fmt.Println(err) }
  debugPrint(s)
}

func (notification Notification) send() {
  var jsonStr = []byte(`{"type": "note", "title": "Youve got mail"}`)

  req, err := http.NewRequest("POST", pushBulletURL, bytes.NewBuffer(jsonStr))
  req.Header.Set("Content-Type", "application/json")
  req.SetBasicAuth(pushBulletToken, "")

  client := &http.Client{}
  resp, err := client.Do(req)

  if err != nil { panic(err) }
  defer resp.Body.Close()

  body, _ := ioutil.ReadAll(resp.Body)

  debugPrint(resp.Status)
  debugPrint(resp.Header)
  debugPrint(string(body))
}

func (rps RedisPubSub) listen(redisConsumer *redis.Client, rec chan []string, incoming chan *Notification) {
  listenToPublish(redisConsumer, &rec, rps.RedisChannel.channel())

  for {
    msg, more := <-rec

    if more {
      if msg[0] == "pmessage" {
        notification := notificationFromMessage(rps.RedisChannel.Base, msg)
        incoming <-notification
      }
    } else {
      redisConsumer.Quit()
      break
    }
  }
}

func main() {
  redisClient = redis.New()
  redisClient.Connect(redisHost, redisPort)

  redisConsumer := newConsumer()
  redisChannel := RedisChannel{"rpi-moteino-collector:motion-detector", "movement"}
  redisPubSub := RedisPubSub{redisChannel}

  incoming := make(chan *Notification)
  rec := make(chan []string)

  go redisPubSub.listen(redisConsumer, rec, incoming)

  for {
    notification := <-incoming
    debugPrint(notification)

    if (! notification.sentToday()) {
      notification.send()
      fmt.Println("INFO: Sent!")
      notification.setSentToday()
    } else {
      fmt.Println("INFO: Already sent today!")
    }
  }
}
