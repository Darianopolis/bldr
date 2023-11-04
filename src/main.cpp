#include <bldr.hpp>

int main(int argc, char* argv[]) try
{
    std::vector<std::string_view> args(argv + 1, argv + argc);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    project_artifactory_t artifactory;
    populate_artifactory(artifactory);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

    for (auto& name : args) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        if (!artifactory.projects.contains(name)) {
            std::cout << "Could not find project with name [" << name << "]\n";
        }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        project_t build;
        generate_build(artifactory, *artifactory.projects.at(name), build);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        debug_project(build);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        build_project(build);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
}
catch (const std::exception& e)
{
    std::cout << "Errror: " << e.what() << '\n';
}
catch (...)
{
    std::cout << "Error\n";
}