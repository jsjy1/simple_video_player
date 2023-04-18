#include "widget.h"

#include <QApplication>


int main(int argc, char *argv[])
{
//    cout<<pthread_self()<<endl;
    QApplication a(argc, argv);
    Widget w;
    w.show();
    return a.exec();
}
