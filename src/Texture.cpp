
#include "Texture.h"
#include <sstream>

using namespace bkengine;

bool Texture::cleanupRegistered = false;
std::map<std::string, std::shared_ptr<TextureWrapper>> Texture::imageCache;
std::map<std::string, std::map<Rect, std::map<Color, std::shared_ptr<TextureWrapper>>>>
Texture::textCache;

static void recalculateRect(Rect &rect, int w, int h)
{
    if (rect.w == 0 && rect.h == 0) {
        Rect windowSize = Core::getInstance()->getWindowSize();
        rect.w = windowSize.w / w;
        rect.h = windowSize.h / h;
    } else if (rect.h == 0) {
        rect.h = (rect.w / w) * h;
    } else if (rect.w == 0) {
        rect.w = (rect.h / h) * w;
    }
}

std::shared_ptr<TextureWrapper> Texture::getCached(const std::string &s)
{
    return imageCache[s];
}

std::shared_ptr<TextureWrapper> Texture::getCached(const std::string &s,
        const Rect &size, const Color &c)
{
    return textCache[s][size][c];
}

bool Texture::hasTextureCached(const std::string &s)
{
    if (imageCache.find(s) == imageCache.end()) {
        return false;
    }

    return true;
}

bool Texture::hasTextureCached(const std::string &s, const Rect &size,
                               const Color &c)
{
    if (textCache.find(s) == textCache.end()) {
        return false;
    }

    if (textCache[s].find(size) == textCache[s].end()) {
        return false;
    }

    if (textCache[s][size].find(c) == textCache[s][size].end()) {
        return false;
    }

    return true;
}

Texture::Texture() : flip(false), texture(nullptr)
{
    if (!cleanupRegistered) {
        Core::registerCleanup(cleanup);
        cleanupRegistered = true;
    }
}

Texture::Texture(const std::string &fontName, const std::string &text,
                 const Rect &size, const Color &color, TextQuality quality) : Texture()
{
    loadText(fontName, text, size, color, quality);
}

Texture::Texture(const std::string &path, const Rect &size,
                 const Rect &clip) : Texture()
{
    loadImage(path, size, clip);
}


Texture::~Texture()
{
}

int Texture::loadText(const std::string &fontName, const std::string &text,
                      const Rect &size, const Color &color, TextQuality quality)
{
    if (hasTextureCached(text, size, color)) {
        texture = getCached(text, size, color);
        int w, h;
        MANGLE_SDL(SDL_QueryTexture)(texture->get(), NULL, NULL, &w, &h);
        Texture::size = size;
        recalculateRect(Texture::size, w, h);
        Texture::clip = Rect();
        return 0;
    }
    Rect windowSize = Core::getInstance()->getWindowSize();
    TTF_Font *font = Fonts::getFont(fontName, RelativeCoordinates::apply(size, windowSize).h);

    if (!font) {
        font = Fonts::registerFont(fontName, RelativeCoordinates::apply(size, windowSize).h);

        if (!font) {
            Logger::LogError("Texture::loadText(const std::string &=" + fontName +
                             ", const std::string &=" + text + ", const Color &=" + color.toString() +
                             ", const Rect &=" + size.toString() + ")");
            return -3;
        }
    }

    SDL_Surface *textSurface = NULL;

    switch (quality) {
        case TextQuality::SOLID:
            textSurface = MANGLE_SDL(TTF_RenderUTF8_Solid)(font, text.c_str(), { color.r, color.g, color.b, color.a });
            break;

        case TextQuality::BLENDED:
            textSurface = MANGLE_SDL(TTF_RenderUTF8_Blended)(font, text.c_str(), { color.r, color.g, color.b, color.a });
            break;
    }

    if (textSurface == NULL) {
        return -1;
    }

    SDL_Texture *nTexture = MANGLE_SDL(SDL_CreateTextureFromSurface)
                            (Core::getInstance()->getRenderer(),
                             textSurface);

    if (nTexture == NULL) {
        return -2;
    }

    int w = textSurface->w, h = textSurface->h;
    MANGLE_SDL(SDL_FreeSurface)(textSurface);
    texture = textCache[text][size][color] = std::make_shared<TextureWrapper>
              (nTexture);
    Texture::size = size;
    recalculateRect(Texture::size, w, h);
    Texture::clip = Rect();
    return 0;
}

int Texture::loadImage(const std::string &path, const Rect &size,
                       const Rect &clip)
{
    if (hasTextureCached(path)) {
        texture = getCached(path);
        int w, h;
        MANGLE_SDL(SDL_QueryTexture)(texture->get(), NULL, NULL, &w, &h);
        Rect tmpSize = size;
        recalculateRect(tmpSize, w, h);
        Texture::clip = clip;
        Texture::size = tmpSize;
        return 0;
    }

    SDL_Texture *nTexture = NULL;
    SDL_Surface *loadedSurface = IMG_Load(path.c_str());

    if (loadedSurface == NULL) {
        return -1;
    }

    nTexture = MANGLE_SDL(SDL_CreateTextureFromSurface)
               (Core::getInstance()->getRenderer(),
                loadedSurface);

    if (nTexture == NULL) {
        return -2;
    }

    MANGLE_SDL(SDL_FreeSurface)(loadedSurface);
    texture = imageCache[path] = std::make_shared<TextureWrapper>(nTexture);
    Rect tmpSize = size;
    recalculateRect(tmpSize, loadedSurface->w, loadedSurface->h);
    Texture::clip = clip;
    Texture::size = tmpSize;
    return 0;
}

Rect Texture::getSize() const
{
    return size;
}

void Texture::setSize(int w, int h)
{
    if (w != 0) {
        size.w = (float) w;
    }

    if (h != 0) {
        size.h = (float) h;
    }
}

void Texture::setSize(const Rect &rect)
{
    size = rect;
}

int Texture::onRender(const Rect &parentRect, bool flip)
{
    SDL_Renderer *renderer = Core::getInstance()->getRenderer();
    Rect rect = { 0, 0, size.w, size.h };
    Rect windowSize = Core::getInstance()->getWindowSize();
    Rect absoluteSize = RelativeCoordinates::apply(RelativeCoordinates::apply(rect, parentRect), windowSize);
    SDL_Rect sdlRect = absoluteSize.toSDLRect();
    SDL_Rect sdlClip = RelativeCoordinates::apply(clip, texture->getSize()).toSDLRect();
    // SDL_Point point { (int) getSize().x / 2, 0 };

    int ret;
    if (flip) {
        ret = MANGLE_SDL(SDL_RenderCopyEx)(renderer, texture->get(), &sdlClip, &sdlRect, 0,
                                           nullptr,
                                           SDL_FLIP_HORIZONTAL);
    } else {
        ret =  MANGLE_SDL(SDL_RenderCopy)(renderer, texture->get(), &sdlClip, &sdlRect);
    }

    return ret;
}

void Texture::cleanup()
{
    Logger::LogDebug("Texture::cleanup(): clear caches");
    imageCache.clear();
    textCache.clear();
}
