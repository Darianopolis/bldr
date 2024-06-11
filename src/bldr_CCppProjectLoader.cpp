#include <bldr_Server.hpp>

#include <sol/sol.hpp>

namespace bldr
{
    struct LuaValues
    {
        std::vector<std::string>                     values;
        std::unordered_map<std::string, std::string> options;
    };

    LuaValues GetValues(const sol::object& obj)
    {
        LuaValues values;

        if (obj.is<sol::table>()) {
            auto table = obj.as<sol::table>();
            for (auto&[key, value] : table) {
                if (key.is<sol::string_view>()) {
                    values.options[key.as<std::string>()] = value.as<std::string>();
                }
            }
            int i = 1;
            while (table.get<sol::object>(i)) {
                auto object = table.get<sol::object>(i++);
                if (object.is<sol::string_view>()) {
                    values.values.push_back(object.as<std::string>());
                } else {
                    Error("Illegal non-string argument in get_values");
                }
            }
        } else {
            values.values.push_back(obj.as<std::string>());
        }

        return values;
    }

    void LoadProjectsFromFile(ProjectDatabase& db, const fs::path& file)
    {
        sol::state lua;

        lua.open_libraries(
            sol::lib::base,
            sol::lib::coroutine,
            sol::lib::string,
            sol::lib::io);

        auto default_dir = file.parent_path();

        CCppProject* project = nullptr;

        // TODO:
        lua.set_function("Os", []{});
        lua.set_function("Error", []{});

        lua.set_function("Project", [&](std::string_view name) {
            project = new CCppProject{};
            project->name = std::string(name);
            project->dir = default_dir;

            db.c_cpp_projects.emplace_back(std::unique_ptr<CCppProject>(project));

            return true;
        });

        lua.set_function("Dir", [&](std::string_view name) {
            project->dir = {fs::absolute(default_dir / name).string()};
        });

        lua.set_function("Compile", [&](const sol::object& obj) {
            auto values = GetValues(obj);
            FileType type = FileType::Auto;
            if (values.options.contains("type")) {
                type = FileTypeFromString(values.options.at("type"));
            }

            for (auto& value : values.values) {
                project->sources.emplace_back(value, type);
            }
        });

        lua.set_function("Embed", [&](const sol::object& obj) {
            auto values = GetValues(obj);

            for (auto& value : values.values) {
                project->embeds.emplace_back(value, FileType::Auto);
            }
        });

        lua.set_function("Include", [&](const sol::object& obj) {
            auto values = GetValues(obj);
            for (auto& value : values.values) {
                auto path = project->dir / value;
                if (values.options.contains("force") && values.options.at("force") == "true") {
                    project->force_includes.emplace_back(value);
                } else {
                    project->includes.emplace_back(value);
                }
            }
        });

        lua.set_function("LibPath", [&](const sol::object& obj) {
            auto values = GetValues(obj);
            for (auto& value : values.values) {
                project->lib_paths.push_back({ value });
            }
        });

        lua.set_function("Import", [&](const sol::object& obj) {
            auto values = GetValues(obj);
            for (auto& value : values.values) {
                project->import_names.emplace_back(value);
            }
        });

        lua.set_function("Define", [&](const sol::object& obj) {
            auto values = GetValues(obj);
            bool build_scope = true;
            bool import_scope = true;
            if (values.options.contains("scope")) {
                auto scope = values.options.at("scope");
                if (scope == "build") {
                    import_scope = false;
                }
                if (scope == "import") {
                    build_scope = false;
                }
            }
            for (auto& define : values.values) {
                std::string key;
                std::string value;
                auto equals = define.find_first_of('=');
                if (equals != std::string::npos) {
                    key = define.substr(0, equals);
                    value = define.substr(equals + 1);
                } else {
                    key = define;
                }
                if (build_scope)  project->build_defines.emplace_back(key, value);
                if (import_scope) project->defines.emplace_back(key, value);
            }
        });

        lua.set_function("Artifact", [&](const sol::object& obj) {
            auto values = GetValues(obj);
            CCppArtifact artifact{};
            if (values.values.empty()) {
                Error("Missing target path for Artifact");
            }
            artifact.file = values.values.front();
            if (!values.options.contains("type")) {
                Error("Missing type: Console/Window for Artifact");
            }
            auto& type = values.options.at("type");
            if      (type == "Console") { artifact.type = CCppArtifactType::ConsoleExecutable; }
            if      (type == "Window")  { artifact.type = CCppArtifactType::WindowExecutable;  }
            else if (type == "Shared")  { artifact.type = CCppArtifactType::SharedLibrary;     }

            project->artifact = std::move(artifact);
        });

        lua.set_function("Platform", [&](std::string_view str) {
            if (str == "Win32") return true;

            Error("Unrecognized platform: [{}]. Must be one of:", str);
            Error(" - Win32");
            std::exit(1);
        });

        lua.set_function("Link", [&](const sol::object& obj) {
            auto values = GetValues(obj);
            for (auto& value : values.values) {
                project->links.emplace_back(value);
            }
        });

        lua.set_function("Shared", [&](const sol::object& obj) {
            auto values = GetValues(obj);
            for (auto& value : values.values) {
                project->shared_libs.emplace_back(value);
            }
        });

        sol::environment env(lua, sol::create, lua.globals());
        try {
            auto res = lua.safe_script_file(file.string(), &sol::script_throw_on_error);
            if (!res.valid()) {
                sol::error error = res;
                Error("Error returned running bldr file: {}", error.what());
                std::exit(1);
            }
        } catch (const std::exception& e) {
            Error("Exception thrown running bldr file: {}", e.what());
        } catch (...) {
            Error("Unknown error thrown running bldr file");
        }
    }

    void ProjectDatabase::LoadProjects()
    {
        c_cpp_projects.clear();
        for (auto& file : project_files) {
            fs::path path;
            try {
                path = fs::path(file);
            } catch (...) {
                continue;
            }
            if (fs::exists(path)) {
                LoadProjectsFromFile(*this, file);
            }
        }
    }

    CCppProject* ProjectDatabase::FindProjectForName(std::string_view name)
    {
        for (auto& project : c_cpp_projects) {
            if (project->name == name) return project.get();
        }
        return nullptr;
    }
}