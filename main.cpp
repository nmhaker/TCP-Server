#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QByteArray>
#include <QDataStream>

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    for(int i = 0 ; i < argc; i++){
        qDebug() << argv[i] << endl;
    }

    if(argc >= 4)
    {
        QString arg = argv[1];
        if(arg == "send")
        {
            QTcpSocket *tcpSocket = new QTcpSocket(&a);
            QString port = argv[3];
            tcpSocket->connectToHost(QHostAddress(argv[2]), port.toInt());
            if(!tcpSocket->waitForConnected(10*1000)){
                qDebug() << "Connection cant be established to host: " << argv[2] << ", at port: " << port << endl;
            }else{
                QString type(argv[4]);
                if(type == "file")
                {
                    QFile file(argv[5]);
                    if(!file.open(QFile::ReadOnly))
                    {
                        qDebug() << "Cannot open file" << endl;
                    }else
                    {
                        QByteArray dataForWritting = file.readAll();
                        int bytesWritten = tcpSocket->write(dataForWritting);
                        if(!tcpSocket->waitForBytesWritten(10*1000))
                        {
                            qDebug() << "Error no bytes written" << tcpSocket->errorString() << endl;
                        }else
                        {
                            qDebug() << bytesWritten << " bytes written. " << endl;
                        }
                    }
                }else if(type == "string")
                {
                    QByteArray data = argv[5];
                    int bytesWritten = tcpSocket->write(data);
                    if(!tcpSocket->waitForBytesWritten(10*1000))
                    {
                        qDebug() << "Error no bytes written" << tcpSocket->errorString() << endl;
                    }else
                    {
                        qDebug() << bytesWritten << " bytes written. " << endl;
                    }
                }else{
                    qDebug() << "Unknown sending format " << endl;
                }
            }
            tcpSocket->close();
            delete tcpSocket;
        }else if(arg == "receive")
        {
            QTcpServer *tcpServer = new QTcpServer(&a);
            QString port = argv[3];
            if(!tcpServer->listen(QHostAddress(QString(argv[2])),port.toInt())){
                qDebug() << "Error, could not start listening, " << tcpServer->serverError() << endl;
            }else{
                if(tcpServer->waitForNewConnection(10*1000)){
                    QTcpSocket *sock = tcpServer->nextPendingConnection();
                    sock->waitForReadyRead(3000);
                    qDebug() << "Connection established, printing data: " << sock->readAll();
                    sock->close();
                    tcpServer->close();
                    delete sock;
                    delete tcpServer;
                }else{
                    qDebug() << "No incoming connection closing server" << endl;
                    tcpServer->close();
                    delete tcpServer;
                }
            }
        }else
        {
            qDebug() << "Unknown command, try again" << endl;
        }
    }else
    {
            qDebug() << "Unsufficient arguments" << endl;
    }

//    return a.exec();
    return 0;
}
