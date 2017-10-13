#include "serialprotocol.h"

SerialProtocol::SerialProtocol(QSerialPort *serialport, QObject *parent) : QObject(parent)
{
    m_serialport = serialport;
    USART_Operate = USART_CONTROL_MAX;
    USART_State = USART_STATE_READY;
}

unsigned short SerialProtocol::CalcCRC16Byte(unsigned char c, unsigned short crc)
{
    for (int i = 0; i < 8; i++)
    {
        bool bit = ((c >> (7 - i) & 1) == 1);
        bool c15 = ((crc >> 15 & 1) == 1);
        crc <<= 1;
        if (c15 ^ bit) crc ^= 0x1021;
    }
    return crc;
}

unsigned short SerialProtocol::CalcCRC16(QByteArray q, unsigned short len)
{
    ushort crc = 0xFFFF;
    for (int j = 0; j < len; j++) {
        crc = CalcCRC16Byte(q[j], crc);
    }
    return crc;
}

unsigned short SerialProtocol::Encode(USART_CONTROL_TYP control, unsigned short len, const QByteArray &data, QByteArray &encoded_data)
{
    unsigned short head, crc;

    len &= 0x7FF;
    control = (USART_CONTROL_TYP)((unsigned short)control & 0x1F);
    head = (control << 11) | len;
    crc = CalcCRC16(data, len);

    encoded_data.clear();
    encoded_data.resize(len + 6);

    encoded_data[0] = 0x02;
    encoded_data[1] = head >> 8;
    encoded_data[2] = head & 0xff;
    encoded_data.replace(3, len, data);
    encoded_data[len + 3] = crc >> 8;
    encoded_data[len + 4] = crc & 0xff;
    encoded_data[len + 5] = 0x03;

    return len + 6;
}

/*
 * 0 -- ok
 * 1 -- error head
 * 2 -- error operator
 * 3 -- error length
 * 4 -- error crc
 * 5 -- error end
 * 6 -- error data
 */
unsigned short SerialProtocol::Decode(const QByteArray &data, unsigned short len, USART_CONTROL_TYP &op, unsigned short &decoded_len, QByteArray &decoded_data)
{
    if (len < 6) return 6;

    op = (USART_CONTROL_TYP)(data[1] >> 3);
    decoded_len = ((data[1] & 0x7) << 8) | data[2];

    if ((unsigned char)data[0] != 0x02) return 1;
    if (op >= USART_CONTROL_MAX) return 2;
    if (decoded_len + 6 != len) return 3;
    decoded_data = data.mid(3, decoded_len + 2);
    if (CalcCRC16(decoded_data, decoded_len + 2) != 0) return 4;
    if ((unsigned char)data[len - 1] != 0x03) return 5;


    return 0;
}

void SerialProtocol::procReadyRead()
{
    QByteArray block = m_serialport->readAll();
    static int cnt = 0;
    foreach (char nowchar, block)
    {
        if (USART_State == USART_STATE_READY)
        {
            if (nowchar != 0x02)
            {
                qDebug("Error start frame.");
                cnt++;
            }
            else
            {
                USART_State = USART_STATE_WAITHEADHIGH;
            }
        }
        else if (USART_State == USART_STATE_WAITHEADHIGH)
        {
            if (nowchar >> 3 >= USART_CONTROL_MAX)
            {
                qDebug("Error operator.");
                cnt++;
                USART_State = USART_STATE_READY;
            }
            else
            {
                USART_Head = nowchar;
                USART_State = USART_STATE_WAITHEADLOW;
            }
        }
        else if (USART_State == USART_STATE_WAITHEADLOW)
        {
            USART_Head = (USART_Head << 8) | nowchar;
            USART_Operate = (USART_CONTROL_TYP)((USART_Head >> 11) & 0x1F);
            USART_DataLen = USART_Head & 0x7FF;
            USART_RecvBuff.clear();
            if (USART_DataLen == 0)
            {
                USART_State = USART_STATE_WAITCRCHIGH;
            }
            else
            {
                USART_State = USART_STATE_WAITDATA;
            }
            USART_CRC = 0xffff;
        }
        else if (USART_State == USART_STATE_WAITDATA)
        {
            USART_RecvBuff.push_back(nowchar);
            USART_CRC = CalcCRC16Byte(nowchar, USART_CRC);

            if (USART_RecvBuff.length() >= USART_DataLen)
            {
                USART_State = USART_STATE_WAITCRCHIGH;
            }
        }

        else if (USART_State == USART_STATE_WAITCRCHIGH)
        {
            USART_CRC = CalcCRC16Byte(nowchar, USART_CRC);
            USART_State = USART_STATE_WAITCRCLOW;
        }
        else if (USART_State == USART_STATE_WAITCRCLOW)
        {
            USART_CRC = CalcCRC16Byte(nowchar, USART_CRC);
            if (USART_CRC != 0)
            {
                qDebug("CRC check failed.");
                cnt++;
                USART_State = USART_STATE_READY;
                USART_Operate = USART_CONTROL_MAX;
            }
            else
            {
              USART_State = USART_STATE_WAITTAIL;
            }
        }
        else if (USART_State == USART_STATE_WAITTAIL)
        {
            if (nowchar != 0x03)
            {
                qDebug("Error end frame.");
                cnt++;
                USART_Operate = USART_CONTROL_MAX;
            }
            else
            {
                //qDebug("OK");
                emit pushNewMsg(USART_Operate, USART_RecvBuff);
            }
            USART_State = USART_STATE_READY;
        }
    }
}
