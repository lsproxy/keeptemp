#include "SPI.h"
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"

//测试的显示屏
#define TFT_DC 9
#define TFT_CS 10
Adafruit_ILI9341 m_image = Adafruit_ILI9341(TFT_CS, TFT_DC);

//重定义类型
using Image = Adafruit_ILI9341;
using uint = unsigned int;

//枚举开关状态
enum Switch{ON,OFF};

//ESP8266 IO定义如下命名空间
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

//像素结构体，传递绘制数据
using Pixel = struct Pixel
{
    uint x;
    uint y;
    uint high;
    uint wide;
    uint size;
    uint color;
};

//设置绘制背景颜色区域
void drawZone(uint x, uint y, uint high, uint wide, uint color)
{
    for(uint i = y; i < y + high; i++)
    {
        m_image.drawFastHLine(x, i, wide, color);
    }
}

// 定制绘制图形
void draw_digital(Pixel pixel, char* str, uint var)
{
    drawZone(pixel.x + 70,pixel.y,pixel.high,pixel.wide,ILI9341_BLACK);
    m_image.setCursor(pixel.x,pixel.y);
    m_image.setTextColor(pixel.color);
    m_image.setTextSize(pixel.size);
    if(str != NULL);
      m_image.print(str);
    m_image.print(var);
} 

// 加热闪烁logo
void logo_heat()
{
    drawZone(142,70,17,17,ILI9341_BLACK);
    m_image.fillCircle(150 , 78 , 7, ILI9341_RED);
    drawZone(142,70,17,17,ILI9341_BLACK);
    delay(600);
    m_image.fillCircle(150 , 78 , 7, ILI9341_RED);
}

//制冷闪烁logo
void logo_cool()
{
    drawZone(142,70,17,17,ILI9341_BLACK);
    m_image.fillCircle(150 , 78 , 7, ILI9341_GREEN);
    drawZone(142,70,17,17,ILI9341_BLACK);
    delay(600);
    m_image.fillCircle(150 , 78 , 7, ILI9341_GREEN);
}

//关闭logo图形
void logo_clear()
{
    drawZone(142,70,17,17,ILI9341_BLACK);
}

//Sensor类,测试完毕，OK
class Sensor
{
public:
    Sensor(const uint& pin);    //构造
    Sensor(){}  //构造
    ~Sensor();  //析构

    void on();      //开
    void off();     //关

    Switch value(); //获取当前Sensor状态
    void set_pin(const uint& pin);  //设置Sensor端口

private:
    uint m_pin;
    Switch status;  //存储状态
};

// Sensor类方法具体实现
Sensor::Sensor(const uint& pin)
{
    set_pin(pin);
    off();
}

Sensor::~Sensor()
{
    off();
}

void 
Sensor::on() 
{
    digitalWrite(m_pin, HIGH);
    status = ON;
}

void 
Sensor::off()
{
    digitalWrite(m_pin, LOW);
    status = OFF;
}

void 
Sensor::set_pin(const uint& pin)
{
    m_pin = pin;
    pinMode(pin,OUTPUT);
    off();
}

Switch 
Sensor::value() 
{
    return status;
}
//-------------------------------------------------------------------

//按键开关类
class Key
{
public:
    Key(const uint& pin);   //构造
    Key(){} //默认构造
    Switch detect();    //检测按键
    void set_pin(const uint& pin);  //设置按键端口

private:
    uint m_pin;
};

//Key类具体实现
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
//-------------------------------------------------------------------


//恒温引擎
class StableEngine
{
public:
    StableEngine(); //构造
    ~StableEngine();    //析构
    void show();    //运行程序
    void bind_sensor(uint heat, uint cool);     //绑定恒温装置端口  
    void bind_key(uint add, uint sub, uint sure);   //绑定按键端口
    void bind_tempdetect(uint pin);     //绑定温度传感端口
    void bind_image(Image* image);  //绑定图形绘制对象
    uint getTemp();     //从传感器获取当前温度

private:
    void key_detect();  //按键检测
    void stableTemp();  //恒温
    void display();     //输出动态信息到显示屏

    //预定义传感器对象
    Sensor* m_heat;
    Sensor* m_cool;

    //预定义开关类对象
    Key* key_add;
    Key* key_sub;
    Key* key_sure;


    uint buffer;    //温度设置缓冲
    uint pre;    //预设定温度
    uint temp;    //当前温度
    uint sensor_pin;    //温度传感器pin

    Image* m_image;     //图像绘制对象

    const uint uplimit = 50;    //默认可设置温度上限
    const uint downlimit = 5;   //默认可设置温度下限

    bool lock;      //用于模拟升温的变量，其作用为，
                    //当已获取传感器温度且与与设定温度有差异则锁定从传感器获取温度，
                    //使用以获取温度值模拟恒温过程。
};

//StableEngine 具体实现
StableEngine::StableEngine()
{
    lock = false;
    temp = getTemp();
    buffer = pre = temp;
}

StableEngine::~StableEngine()
{
    free(m_heat);
    free(m_cool);

    free(key_add);
    free(key_sub);
    free(key_sure);
}

uint 
StableEngine::getTemp()
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
StableEngine::show()
{
    while(1)
    {
        this->temp = getTemp(); 
        this->key_detect();
        this->stableTemp();
        this->display();
    }
}

void 
StableEngine::bind_sensor(uint heat, uint cool)
{
    m_heat = (Sensor* )malloc(sizeof(Sensor));
    m_heat->set_pin(heat);

    m_cool = (Sensor* )malloc(sizeof(Sensor));
    m_cool->set_pin(cool);
}

void 
StableEngine::bind_key(uint add, uint sub, uint sure)
{
    key_add = (Key* )malloc(sizeof(Key));
    key_add->set_pin(add);

    key_sub = (Key* )malloc(sizeof(Key));
    key_sub->set_pin(sub);

    key_sure = (Key* )malloc(sizeof(Key));
    key_sure->set_pin(sure);

}

void 
StableEngine::bind_tempdetect(uint pin)
{
    sensor_pin = pin;
}

void
StableEngine::bind_image(Image* image)
{
    m_image = image;
}

void 
StableEngine::key_detect()
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

void 
StableEngine::stableTemp()
{
    static uint count = 0;

    if(pre != temp)
    {
        lock = true;
        //仿真加热时间10 * 10
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
            case 1: 
            {
                temp++;
                m_cool->off();
                m_heat->on();
                logo_heat();
                break;
            }
            //制冷
            case 0: 
            {
                temp--;
                m_heat->off();
                m_cool->on();
                logo_cool();
                break;
            }
            }
        }
    }
    else
    {
        lock = false;
        m_heat->off();
        m_cool->off();
        logo_clear();
    }
        
}

void 
StableEngine::display()
{
    //打印输出相关信息到控制串口
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
    Pixel temp_image = {10, 70, 18, 30, 2, ILI9341_GREEN};
    draw_digital(temp_image, "TEMP: ", temp);
    //绘制预设温度
    Pixel pre_image = {10, 100, 18, 30, 2, ILI9341_GREEN};
    draw_digital(pre_image, "SET : ", buffer);
}
//-------------------------------------------------------------------

//初始化，只运行一次
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

// 循环运行
void loop() {
    StableEngine temp;
    temp.bind_key(4, 5, 6);
    temp.bind_sensor(2, 3);
    temp.bind_tempdetect(A0);
    temp.bind_image(&m_image);

    temp.show();

}
