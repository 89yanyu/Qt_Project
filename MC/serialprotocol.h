#ifndef SERIALPROTOCOL_H
#define SERIALPROTOCOL_H

#include <QObject>
#include <QByteArray>
#include <QSerialPort>

class SerialProtocol : public QObject
{
    Q_OBJECT
public:
    explicit SerialProtocol(QSerialPort *serialport, QObject *parent = 0);
private:
    typedef enum {
      USART_STATE_READY = 0,
      USART_STATE_WAITHEADHIGH,
      USART_STATE_WAITHEADLOW,
      USART_STATE_WAITDATA,
      USART_STATE_WAITCRCHIGH,
      USART_STATE_WAITCRCLOW,
      USART_STATE_WAITTAIL,
      USART_STATE_MAX
    } USART_STATE_TYP;
    USART_STATE_TYP USART_State;
    QSerialPort *m_serialport;
    unsigned short USART_Head, USART_CRC;

public:
    typedef enum {
      USART_CONTROL_ACK = 0,
      USART_CONTROL_NAK,
      USART_CONTROL_START,
      USART_CONTROL_STOP,
      USART_CONTROL_SETSPEED,
      USART_CONTROL_SETPID,
      USART_CONTROL_STATE,
      USART_CONTROL_DEBUG,
      USART_CONTROL_MAX
    } USART_CONTROL_TYP;
    static unsigned short Encode(USART_CONTROL_TYP control, unsigned short len, const QByteArray &data, QByteArray &encoded_data);
    static unsigned short Decode(const QByteArray &data, unsigned short len, USART_CONTROL_TYP &op, unsigned short &decoded_len, QByteArray &decoded_data);

private:
    USART_CONTROL_TYP USART_Operate;
    QByteArray USART_RecvBuff;
    unsigned short USART_DataLen;
    static unsigned short CalcCRC16(QByteArray q, unsigned short len);
    static unsigned short CalcCRC16Byte(unsigned char c, unsigned short crc);

signals:
    void pushNewMsg(int, QByteArray);
public slots:
    void procReadyRead();
};

#endif // SERIALPROTOCOL_H
