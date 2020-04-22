#pragma once

#include <Exception.hpp>

namespace obe::Graphics::Exceptions
{
    class ReadOnlyTexture : public Exception
    {
    public:
        ReadOnlyTexture(std::string_view method, DebugInfo info)
            : Exception("ReadOnlyTexture", info)
        {
            this->error(
                "Impossible to call method Texture::{} when Texture is in readonly-mode",
                method);
        }
    };

    class InvalidColorName : public Exception
    {
    public:
        InvalidColorName(std::string_view color, DebugInfo info)
            : Exception("InvalidColorName", info)
        {
            this->error(
                "Impossible to find a color with the following name : '{}'", color);
        }
    };

    class CanvasElementAlreadyExists : public Exception
    {
    public:
        CanvasElementAlreadyExists(std::string_view id, std::string_view newElementType,
            std::string_view existingElementType, DebugInfo info)
            : Exception("CanvasElementAlreadyExists", info)
        {
            this->error("Impossible to create a Canvas::{} with id '{}' as there is "
                        "already a Canvas::{} with the same id");
        }
    };
}