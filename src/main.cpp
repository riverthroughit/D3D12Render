#include "Render/D3D12App.h"
#include <QtWidgets/QApplication>
#include"Mesh/MeshLoader.h"

int main(int argc, char *argv[])
{


    //std::vector<Mesh> meshes;
    //MeshLoader::LoadModelsTo("C:/Users/liujx03/source/repos/TotoroEngine/Engine/Resource/Models/Cyborg_Weapon.fbx",
    //    meshes);

    QApplication a(argc, argv);
    D3D12App w;
    w.show();
    return a.exec();
}
