#include "Pragma/Assets/ImageLoader.h"

#include <fstream>
#include <sstream>
#include <string>

namespace
{
std::string NextToken(std::istream& input)
{
    std::string token;

    while (input >> token)
    {
        if (!token.empty() && token[0] == '#')
        {
            std::string ignoredLine;
            std::getline(input, ignoredLine);
            continue;
        }

        return token;
    }

    return {};
}
}

namespace Pragma::Assets
{
Pragma::Core::Result<ImageAssetData> LoadPpmImage(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input.is_open())
    {
        return Pragma::Core::Result<ImageAssetData>::Failure("Failed to open image file: " + path.string());
    }

    const std::string magic = NextToken(input);
    if (magic != "P3")
    {
        return Pragma::Core::Result<ImageAssetData>::Failure("Only ASCII PPM (P3) textures are currently supported: " + path.string());
    }

    const std::string widthToken = NextToken(input);
    const std::string heightToken = NextToken(input);
    const std::string maxValueToken = NextToken(input);

    if (widthToken.empty() || heightToken.empty() || maxValueToken.empty())
    {
        return Pragma::Core::Result<ImageAssetData>::Failure("PPM header is incomplete: " + path.string());
    }

    const int width = std::stoi(widthToken);
    const int height = std::stoi(heightToken);
    const int maxValue = std::stoi(maxValueToken);

    if (width <= 0 || height <= 0 || maxValue <= 0 || maxValue > 255)
    {
        return Pragma::Core::Result<ImageAssetData>::Failure("PPM header contains invalid dimensions or max value: " + path.string());
    }

    ImageAssetData image;
    image.Width = static_cast<std::uint32_t>(width);
    image.Height = static_cast<std::uint32_t>(height);
    image.Pixels.resize(static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4u);

    for (std::size_t pixelIndex = 0; pixelIndex < static_cast<std::size_t>(width) * static_cast<std::size_t>(height); ++pixelIndex)
    {
        const std::string redToken = NextToken(input);
        const std::string greenToken = NextToken(input);
        const std::string blueToken = NextToken(input);

        if (redToken.empty() || greenToken.empty() || blueToken.empty())
        {
            return Pragma::Core::Result<ImageAssetData>::Failure("PPM pixel data is incomplete: " + path.string());
        }

        image.Pixels[pixelIndex * 4u + 0u] = static_cast<std::uint8_t>(std::stoi(redToken));
        image.Pixels[pixelIndex * 4u + 1u] = static_cast<std::uint8_t>(std::stoi(greenToken));
        image.Pixels[pixelIndex * 4u + 2u] = static_cast<std::uint8_t>(std::stoi(blueToken));
        image.Pixels[pixelIndex * 4u + 3u] = 255u;
    }

    return Pragma::Core::Result<ImageAssetData>::Success(std::move(image));
}
}
