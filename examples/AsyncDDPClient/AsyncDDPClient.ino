#include <WiFi.h>
#include <ESPAsyncDDP.h>
#include <ArduinoJson.h>
#include <string>
#include <vector>

const char *ssid = "..........";
const char *password = "..........";

ESPAsyncDDP ddp;
StaticJsonDocument<1024> json;

std::vector<uint8_t> query_cb(ddp_header my_ddp_header)
{
    std::vector<uint8_t> data;

    switch (my_ddp_header.id)
    {
    case DDP_ID_STATUS:
    {
        json.clear();
        JsonObject status = json.createNestedObject("status");
        status["mac"] = WiFi.macAddress();
        status["man"] = "HX2003";
        status["mod"] = "lightbulb";
        status["ver"] = "v0";
        status["push"] = false;
        status["ntp"] = false;

        std::string str;
        serializeJson(json, str);
        data.assign(str.begin(), str.end());

        break;
    }
    case DDP_ID_CONFIG:
        break;
    case DDP_ID_CONTROL:
        break;
    default:
        break;
    }

    return data;
}

void write_cb(ddp_header my_ddp_header, std::vector<uint8_t> &data)
{
    Serial.printf("length: %i\n", data.size());
}

void setup()
{
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    if (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.println("WiFi Failed");
        while (1)
        {
            delay(1000);
        }
    }

    Serial.print("UDP Listening on IP: ");
    Serial.println(WiFi.localIP());

    if (!ddp.begin())
    {
        Serial.print("Setup failed");
    }

    ddp.register_query_cb(query_cb);
    ddp.register_write_cb(write_cb);
}

void loop()
{
    ddp.handle();
}