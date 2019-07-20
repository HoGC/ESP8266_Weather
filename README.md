# ESP8266_Weather
## ESP8266 NONOS SDK  天气预报  

* 实时更新时间
* 每十分钟实时更新一次天气
* 支持smartconfig
* 可网页配置更改预报城市及WIFI

<img src="./weather.jpg"  height="160" width="300">  

### 使用方法

1. 注册[心知天气](https://www.seniverse.com/signup)
2. 得到产品私钥(免费版)
3. 替换user_main.c代码中的私钥
4. 编译下载bin文件
5. 下载html文件,地址为0xD0000,文件选择所有类型选择html/0xD0000.html
6. 连续接通电源三次，进入配置模式
7. 使用手机或电脑连接WiFi：weather
8. 浏览器输入：192.168.4.1
9. 填写预报城市拼音及AP信息并确认

