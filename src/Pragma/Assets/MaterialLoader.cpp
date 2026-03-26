#include "Pragma/Assets/MaterialLoader.h"

#include <array>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
std::string Trim(std::string value)
{
    const std::size_t first = value.find_first_not_of(" \t\r\n");
    if (first == std::string::npos)
    {
        return {};
    }

    const std::size_t last = value.find_last_not_of(" \t\r\n");
    return value.substr(first, last - first + 1);
}

std::vector<std::string> SplitCommaSeparated(const std::string& value)
{
    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string part;
    while (std::getline(stream, part, ','))
    {
        parts.push_back(Trim(part));
    }

    return parts;
}

std::array<float, 4> ParseColor4(const std::string& value)
{
    const std::vector<std::string> parts = SplitCommaSeparated(value);
    if (parts.size() != 4)
    {
        throw std::runtime_error("Expected a 4-component color.");
    }

    return
    {
        std::stof(parts[0]),
        std::stof(parts[1]),
        std::stof(parts[2]),
        std::stof(parts[3])
    };
}

bool ParseBool(const std::string& value)
{
    const std::string trimmed = Trim(value);
    if (trimmed == "true" || trimmed == "1")
    {
        return true;
    }
    if (trimmed == "false" || trimmed == "0")
    {
        return false;
    }

    throw std::runtime_error("Expected a boolean value.");
}

std::string FormatBool(const bool value)
{
    return value ? "true" : "false";
}

std::string FormatColor4(const float color[4])
{
    return std::to_string(color[0]) + "," + std::to_string(color[1]) + "," + std::to_string(color[2]) + "," + std::to_string(color[3]);
}
}

namespace Pragma::Assets
{
MaterialAssetData LoadMaterialAsset(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input.is_open())
    {
        throw std::runtime_error("Failed to open material asset.");
    }

    MaterialAssetData material;
    bool versionDefined = false;
    std::string line;
    std::size_t lineNumber = 0;

    auto parseError = [&](const std::string& message) -> std::runtime_error
    {
        return std::runtime_error(
            "Material parse error in '" + path.string() + "' at line " + std::to_string(lineNumber) + ": " + message);
    };

    while (std::getline(input, line))
    {
        ++lineNumber;
        line = Trim(line);
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        const std::size_t separatorIndex = line.find('=');
        if (separatorIndex == std::string::npos)
        {
            throw parseError("line is missing '=' separator.");
        }

        const std::string key = Trim(line.substr(0, separatorIndex));
        const std::string value = Trim(line.substr(separatorIndex + 1));

        if (key == "version")
        {
            const std::uint32_t version = static_cast<std::uint32_t>(std::stoul(value));
            if (version != 1)
            {
                throw parseError("unsupported material asset version.");
            }
            versionDefined = true;
        }
        else if (key == "base_color")
        {
            const auto color = ParseColor4(value);
            for (std::size_t i = 0; i < color.size(); ++i)
            {
                material.BaseColor[i] = color[i];
            }
        }
        else if (key == "roughness")
        {
            material.Roughness = std::stof(value);
        }
        else if (key == "use_albedo_texture")
        {
            material.UseAlbedoTexture = ParseBool(value);
        }
        else if (key == "albedo_texture")
        {
            material.AlbedoTextureAsset.Value = value;
        }
        else
        {
            throw parseError("unknown material key '" + key + "'.");
        }
    }

    if (!versionDefined)
    {
        throw std::runtime_error("Material parse error in '" + path.string() + "': missing required version.");
    }

    return material;
}

void SaveMaterialAsset(const MaterialAssetData& material, const std::filesystem::path& path)
{
    std::ofstream output(path, std::ios::trunc);
    if (!output.is_open())
    {
        throw std::runtime_error("Failed to open material asset for writing.");
    }

    output << "# Pragma material asset\n";
    output << "version=1\n";
    output << "base_color=" << FormatColor4(material.BaseColor) << '\n';
    output << "roughness=" << material.Roughness << '\n';
    output << "use_albedo_texture=" << FormatBool(material.UseAlbedoTexture) << '\n';
    if (!material.AlbedoTextureAsset.empty())
    {
        output << "albedo_texture=" << material.AlbedoTextureAsset.Value << '\n';
    }
}
}
