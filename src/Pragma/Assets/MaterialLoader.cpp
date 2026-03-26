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

std::array<float, 3> ParseColor3(const std::string& value)
{
    const std::vector<std::string> parts = SplitCommaSeparated(value);
    if (parts.size() != 3)
    {
        throw std::runtime_error("Expected a 3-component color.");
    }

    return
    {
        std::stof(parts[0]),
        std::stof(parts[1]),
        std::stof(parts[2])
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

std::string FormatColor3(const float color[3])
{
    return std::to_string(color[0]) + "," + std::to_string(color[1]) + "," + std::to_string(color[2]);
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
    std::uint32_t version = 0;
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
            version = static_cast<std::uint32_t>(std::stoul(value));
            if (version != 1 && version != 2 && version != 3)
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
        else if (key == "emissive_color")
        {
            const auto color = ParseColor3(value);
            for (std::size_t i = 0; i < color.size(); ++i)
            {
                material.EmissiveColor[i] = color[i];
            }
        }
        else if (key == "roughness")
        {
            material.Roughness = std::stof(value);
        }
        else if (key == "metallic")
        {
            material.Metallic = std::stof(value);
        }
        else if (key == "ambient_occlusion")
        {
            material.AmbientOcclusion = std::stof(value);
        }
        else if (key == "use_albedo_texture")
        {
            material.UseAlbedoTexture = ParseBool(value);
        }
        else if (key == "emissive_intensity")
        {
            material.EmissiveIntensity = std::stof(value);
        }
        else if (key == "use_normal_texture")
        {
            if (version < 3)
            {
                throw parseError("use_normal_texture requires material asset version 3.");
            }
            material.UseNormalTexture = ParseBool(value);
        }
        else if (key == "use_orm_texture")
        {
            if (version < 3)
            {
                throw parseError("use_orm_texture requires material asset version 3.");
            }
            material.UseOrmTexture = ParseBool(value);
        }
        else if (key == "use_emissive_texture")
        {
            if (version < 3)
            {
                throw parseError("use_emissive_texture requires material asset version 3.");
            }
            material.UseEmissiveTexture = ParseBool(value);
        }
        else if (key == "normal_strength")
        {
            if (version < 3)
            {
                throw parseError("normal_strength requires material asset version 3.");
            }
            material.NormalStrength = std::stof(value);
        }
        else if (key == "albedo_texture")
        {
            material.AlbedoTextureAsset.Value = value;
        }
        else if (key == "normal_texture")
        {
            if (version < 3)
            {
                throw parseError("normal_texture requires material asset version 3.");
            }
            material.NormalTextureAsset.Value = value;
        }
        else if (key == "orm_texture")
        {
            if (version < 3)
            {
                throw parseError("orm_texture requires material asset version 3.");
            }
            material.OrmTextureAsset.Value = value;
        }
        else if (key == "emissive_texture")
        {
            if (version < 3)
            {
                throw parseError("emissive_texture requires material asset version 3.");
            }
            material.EmissiveTextureAsset.Value = value;
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
    output << "version=3\n";
    output << "base_color=" << FormatColor4(material.BaseColor) << '\n';
    output << "emissive_color=" << FormatColor3(material.EmissiveColor) << '\n';
    output << "roughness=" << material.Roughness << '\n';
    output << "metallic=" << material.Metallic << '\n';
    output << "ambient_occlusion=" << material.AmbientOcclusion << '\n';
    output << "use_albedo_texture=" << FormatBool(material.UseAlbedoTexture) << '\n';
    output << "use_normal_texture=" << FormatBool(material.UseNormalTexture) << '\n';
    output << "use_orm_texture=" << FormatBool(material.UseOrmTexture) << '\n';
    output << "use_emissive_texture=" << FormatBool(material.UseEmissiveTexture) << '\n';
    output << "emissive_intensity=" << material.EmissiveIntensity << '\n';
    output << "normal_strength=" << material.NormalStrength << '\n';
    if (!material.AlbedoTextureAsset.empty())
    {
        output << "albedo_texture=" << material.AlbedoTextureAsset.Value << '\n';
    }
    if (!material.NormalTextureAsset.empty())
    {
        output << "normal_texture=" << material.NormalTextureAsset.Value << '\n';
    }
    if (!material.OrmTextureAsset.empty())
    {
        output << "orm_texture=" << material.OrmTextureAsset.Value << '\n';
    }
    if (!material.EmissiveTextureAsset.empty())
    {
        output << "emissive_texture=" << material.EmissiveTextureAsset.Value << '\n';
    }
}
}
