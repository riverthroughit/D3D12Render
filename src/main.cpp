#include "Render/D3D12App.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    D3D12App w;
    w.show();
    return a.exec();
}
