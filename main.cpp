#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QFile>
#include <QByteArray>
#include <QDataStream>
#include <QThread>
#include <iostream>

using namespace std;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    for(int i = 0 ; i < argc; i++){
        qDebug() << argv[i] << endl;
    }

    if(argc >= 4)
    {
        //Getting first argument for application
        QString arg = argv[1];
        //Sending mode
        if(arg == "send")
        {
            QTcpSocket *tcpSocket = new QTcpSocket(&a);
            QString port = argv[3];
            tcpSocket->connectToHost(QHostAddress(argv[2]), port.toInt());
            if(!tcpSocket->waitForConnected(10*1000)){
                qDebug() << "Connection cant be established to host: " << argv[2] << ", at port: " << port << endl;
            }else{
                //Sending mode = file
                QString type(argv[4]);
                if(type == "file")
                {
                    QFile file(argv[5]);
                    if(!file.open(QFile::ReadOnly))
                    {
                        qDebug() << "Cannot open file" << endl;
                    }else
                    {
                        qDebug() << "File size in bytes: " << file.size() << endl;
                        int counter = 0;
                        //Divide file into chunks of 300KB
                        int chunkSize = 3 * 100000;
                        while(counter < file.size()){
                                if(!file.seek(counter)){
                                    qDebug() << "Cant seek the file to : " << counter << endl;
                                }else{
                                    QByteArray buffer = file.read(chunkSize);
                                    if(!buffer.isEmpty()){
                                        int bytesWritten = tcpSocket->write(buffer);
                                        counter += bytesWritten;
                                        if(!tcpSocket->waitForBytesWritten(10*1000))
                                        {
                                            qDebug() << "Error no bytes written" << tcpSocket->errorString() << endl;
                                        }else
                                        {
                                            qDebug() << "Bytes for writting: " << buffer.size() << ", " << bytesWritten << " bytes written. " << endl;
                                        }
                                    }else{
                                      qDebug() << "0 bytes read from file, error: " << file.errorString() << endl;
                                      break;
                                    }
                                }
                        }

                    }
                //Sending mode = string
                }else if(type == "string")
                {
                    QByteArray data = argv[5];
                    int bytesWritten = tcpSocket->write(data);
                    if(!tcpSocket->waitForBytesWritten(10000))
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
        //Receiving mode
        }else if(arg == "receive")
        {
            QTcpServer *tcpServer = new QTcpServer(&a);
            QString port = argv[3];
            if(!tcpServer->listen(QHostAddress(QString(argv[2])),port.toInt())){
                qDebug() << "Error, could not start listening, " << tcpServer->serverError() << endl;
            }else{

                QString fileName;
                QString destinationPath;
                //Getting name and path for the new file from user
                bool tryAgain = true;
                while(tryAgain){
                    qDebug() << "Enter filename for the new file (ex: picture.png) :";
                    QTextStream s(stdin);
                    fileName = s.readLine();
                    qDebug() << "Enter destination path: ";
                    QTextStream d(stdin);
                    destinationPath = d.readLine();
                    if (!fileName.isEmpty() && !destinationPath.isEmpty())
                    {
                        qDebug() << "The destination string: " << destinationPath + fileName;
                        tryAgain = false;
                    }else{
                        qDebug() << "You didnt enter filename, try again" << endl;
                    }
                }

                bool working = true;
                while(working){
                    if(tcpServer->waitForNewConnection(10*1000)){
                        QTcpSocket *sock = tcpServer->nextPendingConnection();
                        sock->waitForReadyRead(10*1000);
                        QByteArray receivedData;
                        QFile file_handle(destinationPath + fileName);
                        //While there is bytes available for receiving, receive them and write chunks of min 300KB to file
                        while(sock->bytesAvailable()){
                            qDebug() << sock->bytesAvailable() << " bytes available for writting" << endl;
                            receivedData.append(sock->readAll());
                            if(receivedData.size() > 3 * 100000){
                                if(!file_handle.isOpen()){
                                    if(!file_handle.open(QFile::WriteOnly | QFile::Append | QFile::Text)){
                                        qDebug() << "Could not open file for writing!" << endl;
                                    }else{
                                        file_handle.write(receivedData);
                                        file_handle.flush();
                                        file_handle.waitForBytesWritten(10*1000);
                                        qDebug() << "Written " << receivedData.size() << " to file. In KB = " << receivedData.size() / 1000 << endl;
                                        receivedData.clear();
                                    }
                                }else{
                                    file_handle.write(receivedData);
                                    file_handle.flush();
                                    file_handle.waitForBytesWritten(10*1000);
                                    qDebug() << "Written " << receivedData.size() << " to file. In KB = " << receivedData.size() / 1000 << endl;
                                    receivedData.clear();
                                }
                            }
                            sock->waitForReadyRead(10*1000);
                        }
                        file_handle.close();
                        //In case there is still data in buffer, but data is smaller than 300KB than append that remaining data to file also
                        if(receivedData.size() != 0){
                            qDebug() << "Preparing to write remaining chunks of data" << endl;
                            if(!file_handle.open(QFile::WriteOnly)){
                                qDebug() << "Could not open file for writing!" << endl;
                            }else{
                                file_handle.write(receivedData);
                                file_handle.flush();
                                file_handle.waitForBytesWritten(10*1000);
                                qDebug() << "Written " << receivedData.size() << " to file. In MB = " << receivedData.size() / 1000000 << endl;
                                receivedData.clear();
                                file_handle.close();
                            }
                        }

                        sock->close();
                        delete sock;
//                        file_thread->deleteLater();

                        qDebug() << "Should i wait for other request? (y/n) default = yes" ;
                        char  answer;
                        cin >> answer;
                        if(answer == 'n'){
                            working = false;
                            tcpServer->close();
                            delete tcpServer;
                        }
                    }else{
                        qDebug() << "No incoming connection" << endl;
                        qDebug() << "Should i check for another request? (Yes / No)" ;
                        char answer;
                        cin >> answer;
                        if(answer == 'n'){
                            working = false;
                            tcpServer->close();
                            delete tcpServer;
                        }
                    }
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
