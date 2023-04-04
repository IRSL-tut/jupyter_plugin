#include <cnoid/App>
#include <iostream>

using namespace std;
using namespace cnoid;

int main(int argc, char** argv)
{
    cnoid::App app(argc, argv, "Choreonoid-Jupyter", "Choreonoid");

    if(!app.requirePluginToCustomizeApplication("Jupyter")){
        if(app.error() == App::PluginNotFound){
            auto message = app.errorMessage();
            if(message.empty()){
                cerr << "Jupyter plugin is not found." << endl;
            } else {
                cerr << "Jupyter plugin cannot be loaded.\n";
                cerr << message << endl;
            }
        }
        return 1;
    }

    return app.exec();
}
