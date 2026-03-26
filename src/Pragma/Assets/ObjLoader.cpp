#include "Pragma/Assets/ObjLoader.h"

#include "Pragma/Math/Vector3.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace
{
struct ObjVertexIndices
{
    int PositionIndex = 0;
    int TexCoordIndex = 0;
    int NormalIndex = 0;
};

ObjVertexIndices ParseFaceVertex(const std::string& token)
{
    ObjVertexIndices indices;
    std::size_t start = 0;

    for (std::size_t componentIndex = 0; componentIndex < 3 && start <= token.size(); ++componentIndex)
    {
        const std::size_t slashIndex = token.find('/', start);
        const std::string_view part = slashIndex == std::string::npos
            ? std::string_view(token.data() + start, token.size() - start)
            : std::string_view(token.data() + start, slashIndex - start);

        if (!part.empty())
        {
            const int value = std::stoi(std::string(part));
            if (componentIndex == 0)
            {
                indices.PositionIndex = value;
            }
            else if (componentIndex == 1)
            {
                indices.TexCoordIndex = value;
            }
            else
            {
                indices.NormalIndex = value;
            }
        }

        if (slashIndex == std::string::npos)
        {
            break;
        }

        start = slashIndex + 1;
    }

    return indices;
}

Pragma::Renderer::VertexPCN MakeVertex(
    const Pragma::Math::Vector3& position,
    const Pragma::Math::Vector3& normal,
    const std::array<float, 2>& texcoord) noexcept
{
    return
    {
        { position.X, position.Y, position.Z },
        {
            0.5f + position.X * 0.25f,
            0.5f + position.Y * 0.25f,
            0.5f + position.Z * 0.25f
        },
        { normal.X, normal.Y, normal.Z },
        { texcoord[0], texcoord[1] }
    };
}
}

namespace Pragma::Assets
{
MeshAssetData LoadObjMesh(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input.is_open())
    {
        throw std::runtime_error("Failed to open OBJ mesh file.");
    }

    std::vector<Pragma::Math::Vector3> positions;
    std::vector<Pragma::Math::Vector3> normals;
    std::vector<std::array<float, 2>> texcoords;
    MeshAssetData result;
    std::string line;

    while (std::getline(input, line))
    {
        if (line.empty() || line[0] == '#')
        {
            continue;
        }

        std::istringstream lineStream(line);
        std::string prefix;
        lineStream >> prefix;

        if (prefix == "v")
        {
            Pragma::Math::Vector3 position{};
            lineStream >> position.X >> position.Y >> position.Z;
            positions.push_back(position);
            continue;
        }

        if (prefix == "vt")
        {
            float texcoordU = 0.0f;
            float texcoordV = 0.0f;
            lineStream >> texcoordU >> texcoordV;
            texcoords.push_back({ texcoordU, 1.0f - texcoordV });
            continue;
        }

        if (prefix == "vn")
        {
            Pragma::Math::Vector3 normal{};
            lineStream >> normal.X >> normal.Y >> normal.Z;
            normals.push_back(Pragma::Math::Normalize(normal));
            continue;
        }

        if (prefix == "f")
        {
            std::vector<ObjVertexIndices> faceVertices;
            std::string token;

            while (lineStream >> token)
            {
                const ObjVertexIndices indices = ParseFaceVertex(token);
                if (indices.PositionIndex <= 0 || static_cast<std::size_t>(indices.PositionIndex) > positions.size())
                {
                    throw std::runtime_error("OBJ face references an invalid vertex index.");
                }

                faceVertices.push_back(indices);
            }

            if (faceVertices.size() < 3)
            {
                continue;
            }

            for (std::size_t i = 1; i + 1 < faceVertices.size(); ++i)
            {
                const auto ReadPosition = [&](const ObjVertexIndices& indices) -> const Pragma::Math::Vector3&
                {
                    return positions[static_cast<std::size_t>(indices.PositionIndex - 1)];
                };

                const auto ReadTexcoord = [&](const ObjVertexIndices& indices, const std::size_t fallbackIndex) -> std::array<float, 2>
                {
                    if (indices.TexCoordIndex > 0 && static_cast<std::size_t>(indices.TexCoordIndex) <= texcoords.size())
                    {
                        return texcoords[static_cast<std::size_t>(indices.TexCoordIndex - 1)];
                    }

                    static constexpr std::array<std::array<float, 2>, 3> kFallbackTexcoords =
                    {
                        std::array<float, 2>{ 0.0f, 1.0f },
                        std::array<float, 2>{ 0.0f, 0.0f },
                        std::array<float, 2>{ 1.0f, 0.0f }
                    };

                    return kFallbackTexcoords[fallbackIndex];
                };

                const Pragma::Math::Vector3& position0 = ReadPosition(faceVertices[0]);
                const Pragma::Math::Vector3& position1 = ReadPosition(faceVertices[i]);
                const Pragma::Math::Vector3& position2 = ReadPosition(faceVertices[i + 1]);
                const Pragma::Math::Vector3 generatedNormal =
                    Pragma::Math::Normalize(Pragma::Math::Cross(position1 - position0, position2 - position0));

                const auto ReadNormal = [&](const ObjVertexIndices& indices) -> Pragma::Math::Vector3
                {
                    if (indices.NormalIndex > 0 && static_cast<std::size_t>(indices.NormalIndex) <= normals.size())
                    {
                        return normals[static_cast<std::size_t>(indices.NormalIndex - 1)];
                    }

                    return generatedNormal;
                };

                const std::uint32_t baseIndex = static_cast<std::uint32_t>(result.Vertices.size());
                result.Vertices.push_back(MakeVertex(position0, ReadNormal(faceVertices[0]), ReadTexcoord(faceVertices[0], 0)));
                result.Vertices.push_back(MakeVertex(position1, ReadNormal(faceVertices[i]), ReadTexcoord(faceVertices[i], 1)));
                result.Vertices.push_back(MakeVertex(position2, ReadNormal(faceVertices[i + 1]), ReadTexcoord(faceVertices[i + 1], 2)));

                result.Indices.push_back(baseIndex + 0);
                result.Indices.push_back(baseIndex + 1);
                result.Indices.push_back(baseIndex + 2);
            }
        }
    }

    if (result.Vertices.empty() || result.Indices.empty())
    {
        throw std::runtime_error("OBJ file does not contain any renderable geometry.");
    }

    Pragma::Math::Vector3 minBounds
    {
        result.Vertices[0].Position[0],
        result.Vertices[0].Position[1],
        result.Vertices[0].Position[2]
    };
    Pragma::Math::Vector3 maxBounds = minBounds;

    for (const Pragma::Renderer::VertexPCN& vertex : result.Vertices)
    {
        minBounds.X = std::min(minBounds.X, vertex.Position[0]);
        minBounds.Y = std::min(minBounds.Y, vertex.Position[1]);
        minBounds.Z = std::min(minBounds.Z, vertex.Position[2]);
        maxBounds.X = std::max(maxBounds.X, vertex.Position[0]);
        maxBounds.Y = std::max(maxBounds.Y, vertex.Position[1]);
        maxBounds.Z = std::max(maxBounds.Z, vertex.Position[2]);
    }

    result.LocalBoundsCenter =
    {
        (minBounds.X + maxBounds.X) * 0.5f,
        (minBounds.Y + maxBounds.Y) * 0.5f,
        (minBounds.Z + maxBounds.Z) * 0.5f
    };

    float maxRadiusSquared = 0.0f;
    for (const Pragma::Renderer::VertexPCN& vertex : result.Vertices)
    {
        const Pragma::Math::Vector3 localPosition
        {
            vertex.Position[0],
            vertex.Position[1],
            vertex.Position[2]
        };
        const Pragma::Math::Vector3 offset = localPosition - result.LocalBoundsCenter;
        maxRadiusSquared = std::max(maxRadiusSquared, Pragma::Math::Dot(offset, offset));
    }
    result.LocalBoundsRadius = std::sqrt(maxRadiusSquared);

    return result;
}
}
