#ifndef BKENGINE_ELEMENTTEMPLATES_H
#define BKENGINE_ELEMENTTEMPLATES_H

template <typename T> T &Element::addAnimation(const T &animation)
{
    return addAnimation<T>(std::make_shared<T>(std::move((T &) animation)));
}

template <typename T> T &Element::addAnimation(const std::shared_ptr<T>
        &animation)
{
    animations.push_back(animation);

    if (animations.size() == 1) {
        currentAnimation = 0;
    }

    return *((T *) animations.back().get());
}

template <typename T, typename... Params> T &Element::addAnimation(
    Params... params)
{
    auto animation = std::make_shared<T>(params...);
    animation->setupTextures();
    animation->setupEnvironment();
    animation->backup();
    return addAnimation<T>(animation);
}


template <typename T> T &Element::getAnimation(const std::string &name)
{
    for (auto &animation : animations) {
        if (animation->getName() == name) {
            return *((T *) animation.get());
        }
    }

    Logger::LogCritical("Element::getAnimation(const std::string &=" + name +
                        "): Animation not found");
    throw "Animation not found";
}

template <typename T> T &Element::getAnimation(
    unsigned int index)
{
    if (index < animations.size()) {
        return *((T *) animations[index].get());
    }

    Logger::LogCritical("Element::getAnimation(int=" + std::to_string(
                            index) + "): Animation not found");
    throw "Animation not found";
}


template <typename T> T &Element::getCurrentAnimation()
{
    return getAnimation<T>(currentAnimation);
}

#endif