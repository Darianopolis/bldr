#include "bldr.hpp"

#include <sol/sol.hpp>

struct values_t
{
    std::vector<std::string> values;
    std::unordered_map<std::string, std::string> options;
};

values_t get_values(sol::object obj)
{
    values_t values;

    if (obj.is<sol::table>()) {
        auto table = obj.as<sol::table>();
        for (auto&[key, value] : table) {
            if (key.is<sol::string_view>()) {
                values.options[key.as<std::string>()] = value.as<std::string>();
            }
        }
        int i = 1;
        while (table[i]) {
            values.values.push_back(table[i++].get<std::string>());
        }
    } else {
        values.values.push_back(obj.as<std::string>());
    }

    return values;
}

void populate_artifactory_from_file(project_artifactory_t& artifactory, const std::filesystem::path& file)
{
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    sol::state lua;

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    lua.open_libraries(
        sol::lib::base,
        sol::lib::coroutine,
        sol::lib::string,
        sol::lib::io);

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    auto default_dir = file.parent_path();

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    project_t* project = nullptr;

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    // TODO:
    lua.set_function("Os", []{});
    lua.set_function("Error", []{});
    lua.set_function("Dynamic", []{});
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

    lua.set_function("Project", [&](std::string_view name) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        if (project) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            debug_project(*project);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        }

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        project = new project_t{};
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        project->name = std::string(name);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        project->dir = path_t(default_dir.string());

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        std::cout << "project->name = " << project->name << '\n';
        std::cout << "project = " << project << '\n';
        std::cout << "artifactory.projects.size() = " << artifactory.projects.size() << '\n';
        artifactory.projects.insert({ project->name, project });

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        return true;
    });
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

    lua.set_function("Dir", [&](std::string_view name) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        project->dir = {std::filesystem::absolute(name).string()};
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    });
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

    lua.set_function("Compile", [&](sol::object obj) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        auto values = get_values(obj);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        source_type_t type = source_type_t::automatic;
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        if (values.options.contains("type")) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            auto type_str = values.options.at("type");
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            if      (type_str == "cppm") type = source_type_t::cppm;
            else if (type_str == "cpp")  type = source_type_t::cpp;
            else if (type_str == "c")    type = source_type_t::c;
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        for (auto& value : values.values) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            project->sources.push_back({ {std::move(value), &project->dir}, type });
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    });

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    lua.set_function("Include", [&](sol::object obj) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        auto values = get_values(obj);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        for (auto& value : values.values) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            project->includes.push_back({ std::move(value), &project->dir });
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    });
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

    lua.set_function("Import", [&](sol::object obj) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        auto values = get_values(obj);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        for (auto& value : values.values) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            project->imports.emplace_back(std::move(value));
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    });
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

    lua.set_function("Define", [&](sol::object obj) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        auto values = get_values(obj);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        bool build_scope = true;
        bool import_scope = true;
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        if (values.options.contains("scope")) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            auto scope = values.options.at("scope");
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            if (scope == "build") {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
                import_scope = false;
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        for (auto& define : values.values) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            std::string key;
            std::string value;
            auto equals = define.find_first_of('=');
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            if (equals != std::string::npos) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
                key = define.substr(0, equals);
                value = define.substr(equals + 1);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            } else {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
                key = define;
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            if (build_scope)  project->build_defines.push_back({ key, value });
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            if (import_scope) project->defines.push_back({ key, value });
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        }
    });
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

    lua.set_function("Artifact", [&](sol::object obj) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        auto values = get_values(obj);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        artifact_t artifact{};
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        artifact.path = {values.values.front(), &project->dir};
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        auto& type = values.options.at("type");
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        if      (type == "Console") { artifact.type = artifact_type_t::executable;     artifact.path.path += ".exe"; }
        else if (type == "Shared")  { artifact.type = artifact_type_t::shared_library; artifact.path.path += ".dll"; }

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        project->artifact = std::move(artifact);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    });
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

    lua.set_function("Link", [&](sol::object obj) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        auto values = get_values(obj);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        for (auto& value : values.values) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            project->links.push_back({std::string(value), &project->dir});
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    });
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

    sol::environment env(lua, sol::create, lua.globals());
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    lua.script_file(file.string());
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

    if (project) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        debug_project(*project);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
}

void populate_artifactory(project_artifactory_t& artifactory)
{
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    for (auto& file : std::filesystem::directory_iterator(std::filesystem::current_path())) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        if (file.path().string().ends_with("bldr.lua")) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            std::cout << "Loading bldr file: " << file.path().string() << '\n';
            populate_artifactory_from_file(artifactory, file.path());
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    }
}