#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

//测试的显示屏
#define TFT_DC 9
#define TFT_CS 10
Adafruit_ILI9341 m_image = Adafruit_ILI9341(TFT_CS, TFT_DC);

using Image = Adafruit_ILI9341;
using uint = unsigned int;

enum Switch{ON,OFF};

namespace IO{
    const uint D0 = 16;
    const uint D1 = 5;
    const uint D2 = 4;
    const uint D3 = 0;
    const uint D4 = 2;
    const uint D5 = 14;
    const uint D6 = 12;
    const uint D7 = 13;
    const uint D8 = 15;
    const uint D9 = 3;
    const uint D10 = 1;
};

using Pixel = struct Pixel
{
  uint x;
  uint y;
  uint high;
  uint wide;
  uint size;
  uint color;
};

void drawZone(uint x, uint y, uint high, uint wide, uint color)
{
  for(uint i = y; i < y + high; i++)
  {
    m_image.drawFastHLine(x, i, wide, color);
  }
}

void draw_digital(Pixel pixel, char* str, uint var)
{
    drawZone(pixel.x,pixel.y,pixel.high,pixel.wide,ILI9341_BLACK);
    m_image.setCursor(pixel.x,pixel.y);
    m_image.setTextColor(pixel.color);
    m_image.setTextSize(pixel.size);
    if(str != NULL);
      m_image.print(str);
    m_image.print(var);
} 

//灯类,测试完毕，OK
class Light
{
public:
    Light(const uint& pin);
    Light(){}

    void on();      //亮
    void off();     //暗
    void flash();   //闪烁

    Switch value(); //获取当前led灯状态
    void set_pin(const uint& pin);  //设置led灯端口

private:
    uint m_pin;
    Switch status;
};

Light::Light(const uint& pin)
{
    set_pin(pin);
}

void 
Light::on() 
{
    digitalWrite(m_pin, LOW);
    status = ON;
}

void 
Light::off()
{
    digitalWrite(m_pin, HIGH);
    status = OFF;
}

void 
Light::set_pin(const uint& pin)
{
    m_pin = pin;
    pinMode(pin,OUTPUT);
    digitalWrite(m_pin,HIGH);
    status = OFF;
}

Switch 
Light::value() 
{
    return status;
}

void 
Light::flash()
{
    on();
    delay(300);
    off();
    delay(300);
}



//按键开关
class Key
{
public:
    Key(const uint& pin);
    Key(){}
    Switch detect();    //检测按键
    void set_pin(const uint& pin);  //设置按键端口

private:
    uint m_pin;
};


Key::Key(const uint& pin)
{
    set_pin(pin);
}

Switch 
Key::detect()
{
    if(digitalRead(m_pin) == LOW)
    {
        delay(20);
        if(digitalRead(m_pin) == LOW)
        {
            while(digitalRead(m_pin) == LOW);
        }
        return ON;
    }
    else return OFF;
        
}

void 
Key::set_pin(const uint& pin)
{
    m_pin = pin;
    pinMode(pin,INPUT_PULLUP);
}

class TempEngine
{
public:
    TempEngine();
    ~TempEngine();
    void show();
    void bind_light(uint red, uint green);
    void bind_key(uint add, uint sub, uint sure);
    void bind_sensor(uint pin);
    void bind_image(Image* image);
    uint getTemp();
private:
    void key_detect()
    {
        if(key_add->detect() == ON)
            buffer++;
        if(key_sub->detect() == ON)
            buffer--;
        if(key_sure->detect() == ON)
        {
            pre = buffer;
        }

        if(buffer <= downlimit)
            buffer = downlimit;

        if(buffer >= uplimit)
            buffer = uplimit;
    }

    void light_detect()
    {
        static uint count = 0;
        static uint buffer_tmp = buffer;
        //此时正在设置温度
        if(pre == buffer)
        {
            led_green->off();
        }
        if(buffer != pre)
        {
            //绿灯常量
            led_green->on();
            led_red->off();
            //按键有效时间 30 * 10 ms
            if(count++ < 30)
            {   delay(10);
                if(buffer != buffer_tmp)
                {
                  buffer_tmp = buffer;
                  count = 0;
                }
            }
                
            else
            {
                buffer = pre;
                led_green->flash();
                count = 0;
            }

        }

        //此时温度稳定
        else if(buffer == pre && pre == temp)
        {
            //绿灯闪烁
            led_green->flash();
            led_red->off();
        }

        //正在加热
        else if(pre != temp)
        {
            led_red->on();
        }
        else;
    }

    void stableTemp()
    {
        static uint count = 0;

        if(pre != temp)
        {
            lock = true;
            //仿真加热时间100 * 10
            if(count++ < 10 )
            {
                delay(10);
            }
                
            else
            {
                count = 0;

                switch(uint(pre > temp))
                {
                //加热
                case 1: temp++;break;
                //制冷
                case 0: temp--;break;
                }
            }
        }
        else
            lock = false;
    }

    void display()
    {
        Serial.println("------------------------------------------------------");
        Serial.print("Temp: ");
        Serial.println(temp);
        Serial.print("SET : ");
        Serial.println(pre);
        Serial.print("BUFFER : ");
        Serial.println(buffer);
        Serial.print("LOCK : ");
        Serial.println(lock);
        //绘制温度
        Pixel temp_image = {20, 70, 30, 150, 2, ILI9341_GREEN};
        draw_digital(temp_image, "TEMP: ", temp);
        //绘制预设温度
        Pixel pre_image = {20, 100, 30, 150, 2, ILI9341_GREEN};
        draw_digital(pre_image, "SET : ", buffer);

    }

    Light* led_red;
    Light* led_green;

    Key* key_add;
    Key* key_sub;
    Key* key_sure;

    uint buffer;
    uint pre;
    uint temp;
    uint sensor_pin;
    Image* m_image;
    const uint uplimit = 50;
    const uint downlimit = 5;
    bool lock;
};

TempEngine::TempEngine()
{
    lock = false;
    temp = getTemp();
    buffer = pre = temp;
}

TempEngine::~TempEngine()
{
    free(led_green);
    free(led_red);

    free(key_add);
    free(key_sub);
    free(key_sure);
}

uint 
TempEngine::getTemp()
{
    //返回当前实际温度值
    if(lock == false)
    {
        int analogValue = analogRead(sensor_pin);
        float celsius = 1 / (log(1 / (1023. / analogValue - 1)) / 3950 + 1.0 / 298.15) - 273.15;
        return (uint)(celsius);
    }
    
    return temp;
        
}

void 
TempEngine::show()
{
    while(1)
    {
        this->temp = getTemp(); 
        this->key_detect();
        this->light_detect();
        this->stableTemp();
        this->display();
    }
}

void 
TempEngine::bind_light(uint red, uint green)
{
    led_red = (Light* )malloc(sizeof(Light));
    led_red->set_pin(red);

    led_green = (Light* )malloc(sizeof(Light));
    led_green->set_pin(green);
}

void 
TempEngine::bind_key(uint add, uint sub, uint sure)
{
    key_add = (Key* )malloc(sizeof(Key));
    key_add->set_pin(add);

    key_sub = (Key* )malloc(sizeof(Key));
    key_sub->set_pin(sub);

    key_sure = (Key* )malloc(sizeof(Key));
    key_sure->set_pin(sure);

}

void 
TempEngine::bind_sensor(uint pin)
{
    sensor_pin = pin;
}

void
TempEngine::bind_image(Image* image)
{
    m_image = image;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println("Hello, ESP32!");

  //界面绘制
  m_image.begin();
  m_image.fillScreen(ILI9341_BLACK);
  m_image.setCursor(20, 20);
  m_image.setTextColor(ILI9341_RED);
  m_image.setTextSize(2);
  m_image.println("     Control!       ");
  m_image.println("--------------------");


}

void loop() {
    TempEngine temp;
    temp.bind_key(4, 5, 6);
    temp.bind_light(2, 3);
    temp.bind_sensor(A0);
    temp.bind_image(&m_image);

    temp.show();

}