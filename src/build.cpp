#include "bldr.hpp"

#include <unordered_set>
#include <filesystem>
#include <array>

#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "Windows.h"
#include "shellapi.h"

namespace fs = std::filesystem;

void generate_build(project_artifactory_t& artifactory,  project_t& project, project_t& output)
{
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    std::unordered_set<std::string_view> visited;

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    auto insert_all = [](auto& target, auto& source)
    {
        target.insert_range(target.end(), source);
    };

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    auto collect = [&](this auto&& self, project_t& cur_project)
    {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        if (visited.contains(cur_project.name)) return;
        visited.insert(cur_project.name);

        output.imports.push_back(cur_project.name);

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        insert_all(output.includes,       cur_project.includes);
        insert_all(output.force_includes, cur_project.force_includes);
        insert_all(output.lib_paths,      cur_project.lib_paths);
        insert_all(output.links,          cur_project.links);
        insert_all(output.build_defines,
            &cur_project == &project
                ? cur_project.build_defines
                : cur_project.defines);

        for (auto& import : cur_project.imports) {
            self(*artifactory.projects.at(import));
        }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    };

    collect(project);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

    for (auto&[path, type] : project.sources) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        auto insert_source = [&](const fs::path& file)
        {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            auto src_type = type;
            if (type == source_type_t::automatic) {
                if      (file.extension() == ".cppm") src_type = source_type_t::cppm;
                else if (file.extension() == ".ixx" ) src_type = source_type_t::cppm;
                else if (file.extension() == ".cpp" ) src_type = source_type_t::cpp;
                else if (file.extension() == ".cxx" ) src_type = source_type_t::cpp;
                else if (file.extension() == ".cc"  ) src_type = source_type_t::cpp;
                else if (file.extension() == ".c"   ) src_type = source_type_t::c;
            }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

            if (src_type != source_type_t::automatic) {
                output.sources.push_back({ {file.string()}, src_type });
            }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        };
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

        if (path.path.ends_with("/**")) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            auto iter = fs::recursive_directory_iterator(path.to_fspath(3));
            for (auto& file : iter) insert_source(file.path());
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        } else if (path.path.ends_with("/*")) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            auto iter = fs::directory_iterator(path.to_fspath(2));
            for (auto& file : iter) insert_source(file.path());
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        } else {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            insert_source(path.to_fspath());
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

    output.name = project.name;
    output.artifact = project.artifact;
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
}

void build_project(project_t& project)
{
    auto artifacts_dir = std::filesystem::current_path() / "artifacts";

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
#pragma omp parallel for
    for (uint32_t i = 0; i < project.sources.size(); ++i) {
        auto& source = project.sources[i];

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        program_exec_t info;
        auto dir = artifacts_dir / project.name;
        std::filesystem::create_directories(dir);
        info.working_directory = {dir.string()};
        info.executable = {std::string("cl.exe")};

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        auto arg = [&](auto& exec_info, auto&&... args)
        {
            std::stringstream ss;
            (ss << ... << args);
            exec_info.arguments.push_back(ss.str());
        };
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

        arg(info, "/c");
        arg(info, "/nologo");
        arg(info, "/arch:AVX512");
        arg(info, "/MD");
        arg(info, "/Zc:preprocessor");
        arg(info, "/fp:fast");
        arg(info, "/utf-8");
        arg(info, "/permissive-");
        arg(info, "/O2");
        arg(info, "/INCREMENTAL");

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        if (source.type == source_type_t::c) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            arg(info, "/std:c17");
            arg(info, "/Tc", source.file.to_fspath().string());
        } else {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
            arg(info, "/EHsc");
            arg(info, "/openmp:llvm");
            arg(info, "/Zc:__cplusplus");
            arg(info, "/EHsc");
            arg(info, "/Zc:__cplusplus");
            arg(info, "/std:c++latest");
            if (source.type == source_type_t::cpp) {
                arg(info, "/experimental:module");
                arg(info, "/translateInclude");
            }
            arg(info, "/Tp", source.file.to_fspath().string());
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        }

        for (auto& include : project.includes)       arg(info, "/I",  include.to_fspath().string());
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        for (auto& include : project.force_includes) arg(info, "/FI", include.to_fspath().string());
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        for (auto& define  : project.build_defines) {
            if (define.value.empty()) arg(info, "/D", define.key);
            else                      arg(info, "/D", define.key, "=", define.value);
        }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

        execute_program(info);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    }

    if (project.artifact) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        program_exec_t info;
        info.working_directory = {artifacts_dir.string()};
        info.executable = {std::string("link.exe")};
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

        auto arg = [&](auto& exec_info, auto&&... args)
        {
            std::stringstream ss;
            (ss << ... << args);
            exec_info.arguments.push_back(ss.str());
        };
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

        arg(info, "/nologo");
        arg(info, "/IGNORE:4099");
        arg(info, "/DYNAMICBASE:NO");
        arg(info, "/NODEFAULTLIB:msvcrtd.lib");
        arg(info, "/NODEFAULTLIB:libcmt.lib");
        arg(info, "/NODEFAULTLIB:libcmtd.lib");
        arg(info, "/SUBSYSTEM:CONSOLE");
        arg(info, "/OUT:", project.artifact->path.to_fspath().string());
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        std::filesystem::create_directories(project.artifact->path.to_fspath().parent_path());

        for (auto& lib_path : project.lib_paths) {
            arg(info, "/LIBPATH:", lib_path.to_fspath().string());
        }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

        for (auto& link : project.links) {
            arg(info, link.to_fspath().string());
        }

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        for (auto& import : project.imports) {
            auto dir = artifacts_dir / import;
            if (!std::filesystem::exists(dir)) continue;
            auto iter = std::filesystem::directory_iterator(dir);
            for (auto& file : iter) {
                if (file.path().extension() == ".obj") {
                    auto relative = std::filesystem::relative(file, artifacts_dir).string();
                    arg(info, relative);
                }
            }
        }

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        arg(info, "user32.lib");
        arg(info, "gdi32.lib");
        arg(info, "shell32.lib");
        arg(info, "Winmm.lib");
        arg(info, "Advapi32.lib");
        arg(info, "Comdlg32.lib");
        arg(info, "comsuppw.lib");
        arg(info, "onecore.lib");
std::cout<<__FILE__<<":"<<__LINE__<<'\n';

        execute_program(info);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
}

void execute_program(const program_exec_t& info)
{
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    std::stringstream ss;
    auto cmd = [&](std::string_view value)
    {
        if (value.contains(' ')) {
            ss << '"' << value << "\" ";
        } else {
            ss << value << ' ';
        }
    };

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    cmd(info.executable.to_fspath().string());
    for (auto& arg : info.arguments) cmd(arg);

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    std::string cmd_line = ss.str();
    std::cout << "Running command:\n" << cmd_line << '\n';

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    STARTUPINFOA startup{};
    PROCESS_INFORMATION process{};

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    auto res = CreateProcessA(
        nullptr,
        cmd_line.data(),
        nullptr,
        nullptr,
        false,
        NORMAL_PRIORITY_CLASS,
        nullptr,
        info.working_directory.to_fspath().string().c_str(),
        &startup,
        &process);

std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    if (!res) {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        std::cout << "Error: " << GetLastError() << '\n';
    } else {
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
        WaitForSingleObject(process.hProcess, INFINITE);
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
    }
std::cout<<__FILE__<<":"<<__LINE__<<'\n';
}