#ifndef FILE_SERVER_H
#define FILE_SERVER_H

#include <QObject>

class file_server : public QObject
{
    Q_OBJECT

public:
    explicit file_server(QObject *parent = nullptr);

    void open_wifi();

    void close_wifi();

    void start_file_server();

    void stop_file_server();

signals:


};

#endif // FILE_SERVER_H
