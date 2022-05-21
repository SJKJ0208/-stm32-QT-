 #include "widget.h"
#include "ui_widget.h"
#include<QPushButton>
#include<QDebug>
#include<QVector>
#include<QLabel>
#include "qcustomplot.h"

const int color[10]={
    0xff0000,0x00ff00,0x0000ff,0xff00ff,
    0xff8000,0xc0ff00,0x00ffff,0x8000ff,0x000001,
};

Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget)
{
    ui->setupUi(this);


    first_init();
    my_connect();

}

Widget::~Widget()
{
    delete ui;
}


//信号链接函数
void Widget::my_connect()
{
    connect(ui->btn_cheak,&QPushButton::clicked,this,&Widget::cheak_serial);
    connect(ui->btn_start_end,&QPushButton::clicked,this,&Widget::set_serial);
    connect(&port,&QSerialPort::readyRead,this,&Widget::receiveInfo);

}


void Widget::check()
{
    QByteArray info;
    info = port.readAll();


    qDebug()<<info ;
}

void Widget::first_init()
{
    //作为标志，更新text的标志




    lon = 100;
    rebomm = 0;//这是重载的时候的计算法
    serial_ready = false;
    oscilloscope_ready = false;
    num_point = 0;
    black = 0;//作为间隔用的
    my_black = 5;//作为预期的间隔
}
//检查可用端口并更新
void Widget::cheak_serial()
{
    if(QObject::sender() == ui->btn_cheak)
    {
        ui->com_com->clear();
        //接收
        QList<QSerialPortInfo> avail = QSerialPortInfo::availablePorts();
        foreach (const QSerialPortInfo &s , avail) {
            ui->com_com->addItem(s.portName());
        }
    }

}

void Widget::receiveInfo()//
{
    /*--------------------------------
     *  接受串口信息
     *-------------------------------*/
    QString data;
    QByteArray info = port.readAll();
    QStringList list;

    if(!info.isEmpty())
    {
        if (info.contains('{')){
            data = info;
            list= data.split("{");
            if (list.length() == 2){
                if (frontData.isEmpty()){
                    frontData = list.at(1);
                    frontData.insert(0,'{');
                    return;
                }
                //用于在被截断了的失效数据重现
                oneData = frontData + list.at(0);
                frontData = list.at(1);
                frontData.insert(0,'{');
                qDebug() << "One data: " << oneData;
            }
            //因为够了32位就自动进行一次的，顶多是两个，因为传的东西几乎有20个bit
            if (list.length() == 3){
                oneData = frontData + list.at(0);
                qDebug() << "One data: " << oneData;
                oneData = list.at(1);
                oneData.insert(0,'{');
                qDebug() << "One data: " << oneData;
                frontData = list.at(2);
                frontData.insert(0,'{');
            }
         }
                //避免中文乱码
                QTextCodec *tc = QTextCodec::codecForName("GBK");//找编码器
                QString tmpQStr = tc->toUnicode(info);//
                    QStringList datakeys;
                    QJsonParseError jsonError;
                    QJsonDocument jsonDoc = QJsonDocument::fromJson(info, &jsonError);

                    if (jsonError.error != QJsonParseError::NoError)
                    {
                        //qDebug() << "parse json object failed, " << jsonError.errorString();
                        //ui->listWidget->addItem("parse json object failed!");
                        return;
                    }
                    QJsonObject jsonObj = jsonDoc.object();

                   // qDebug() << "parse json object Successfully";
                    datakeys = jsonObj.keys();      // 获取信息名称
                    qDebug() << datakeys;
                    if(datakeys[0] == "d")
                    {
                        qDebug()<<"危险信号";
                        switch (jsonObj.value(datakeys[0]).toInt()) {
                        case 5:
                            qDebug()<<5;
                            ui->label_war->setText("轮椅有左右倾倒的风险");
                            break;
                        case 6:
                             qDebug()<<6;
                              ui->label_war->setText("轮椅可能已经倾倒");
                            break;
                        case 2:
                             qDebug()<<2;
                              ui->label_war->setText("容易前倾和刹车失灵");
                            break;
                        case 1:
                             qDebug()<<1;
                              ui->label_war->setText("上坡坡度过大");
                            break;
                        case 0:
                            qDebug()<<0;
                             ui->label_war->setText("轮椅姿态正常");
                            break;
                        default:
                            break;
                        }
                    }
                    else if(datakeys[0] == "tm")
                    {
                        qDebug()<<"温度";
                        ui->label_tin_7->setText(QString::number(jsonObj.value(datakeys[0]).toInt()));
                    }
                    else if (datakeys[0] == "sp")
                    {
                        qDebug()<<"速度";
                        ui->label_tin_8->setText(QString::number( jsonObj.value(datakeys[0]).toInt()));

                    }
                    else if (datakeys[0] == "fsar")
                    {
                        qDebug()<<"角度";
                        ui->label_tin_9->setText("下坡角度 ："+QString::number( jsonObj.value(datakeys[0]).toInt()));
                    }
                    else if(datakeys[0] == "syar")
                    {
                        qDebug()<<"角度";
                        ui->label_tin_9->setText("上坡角度 ："+QString::number( jsonObj.value(datakeys[0]).toInt()));
                    }
                        qDebug()<<jsonObj.value(datakeys[0]).toInt();
//                // 向接收区打印
//                ui->textBrowser->append(tmpQStr);

    }
}

//串口设置函数
void Widget::set_serial()
{

    if(ui->btn_start_end->text() == "关闭串口")
    {
        port.close();
        serial_ready = false;
        ui->listWidget->addItem("串口已关闭");
        ui->btn_start_end->setText("打开串口");
        return;
    }

    //恢复旗帜
    flag_error = 0;

    //设置校验位//
    switch (ui->com_jiao->currentIndex()) {
    case 0:
        port.setParity(QSerialPort::NoParity);
        break;
    case 1:
        port.setParity(QSerialPort::OddParity);
        break;
    case 2:
        port.setParity(QSerialPort::EvenParity);
        break;
    default:
        //输出错误
        ui->listWidget->addItem("校验位设置失败！");
        flag_error = 1;
        break;
    }

    //设置数据位//
    switch (ui->com_shu->currentIndex()) {
    case 0://8
        port.setDataBits(QSerialPort::Data8);
        break;
    case 1://7
        port.setDataBits(QSerialPort::Data7);
        break;
    case 2://6
        port.setDataBits(QSerialPort::Data6);
        break;
    case 3://5
        port.setDataBits(QSerialPort::Data5);
        break;
    default:
        //输出错误
        ui->listWidget->addItem("数据位设置失败！");
        flag_error = 1;
        break;
    }

    //设置停止位//
    switch (ui->com_tin->currentIndex()) {
    case 0://1
        port.setStopBits(QSerialPort::OneStop);
        break;
    case 1://1.5
        port.setStopBits(QSerialPort::OneAndHalfStop);
        break;
    case 2://2
        port.setStopBits(QSerialPort::TwoStop);
        break;
    default:
        //输出错误
        ui->listWidget->addItem("停止位设置失败！");
        flag_error = 1;
        break;
    }

    //设置流控制
    port.setFlowControl(QSerialPort::NoFlowControl);

    //设置端口
    QString name = ui->com_com->currentText();
    QString num;
    //提取出来
    for(int i = 3;i < name.size() ;i++)
    {
      num[i-3] = name[i];
    }
    int ch = num.toInt();
    qDebug()<<ch;
    switch (ch) {
    case 1://1
        port.setPortName("COM1");
        break;
    case 2://1
        port.setPortName("COM2");
        break;
    case 3://1
        port.setPortName("COM3");
        break;
    case 4://1
        port.setPortName("COM4");
        break;
    case 5://1
        port.setPortName("COM5");
        break;
    case 6://1
        port.setPortName("COM6");
        break;
    case 7://1
        port.setPortName("COM7");
        break;
    case 8://1
        port.setPortName("COM8");
        break;
    case 9:
        port.setPortName("COM9");
        break;
    case 16:
        port.setPortName("COM16");
        break;
    case 18:
        port.setPortName("COM18");
        break;
    default:
        //输出错误
        ui->listWidget->addItem("端口设置失败！");
        flag_error = 1;
        break;
    }

    //设置波特率
    switch (ui->com_bo->currentIndex()) {
    case 0://115200
        port.setBaudRate(QSerialPort::Baud115200);
        break;
    case 1://9600
        port.setBaudRate(QSerialPort::Baud9600);
        break;
    default:
        //输出错误
        ui->listWidget->addItem("波特率设置失败！");
        flag_error = 1;
        break;
    }

    //与语句，如果前面不满足不执行后边
    if(flag_error == 0 && port.open(QIODevice::ReadWrite))
    {
        //打开串口
        qDebug()<<"串口设置完毕";
        ui->listWidget->addItem("串口已经打开");
        serial_ready = true;

        //打开后，将按钮改一下
        ui->btn_start_end->setText("关闭串口");
    }
    else{
        ui->listWidget->addItem("打开失败，请根据提示进行修复");
    }
}
