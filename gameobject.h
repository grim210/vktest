#ifndef VKTEST_GAMEOBJECT_H
#define VKTEST_GAMEOBJECT_H

class GameObject {
public:
    virtual void Draw(void) = 0;
    virtual void Rebuild(void) = 0;
    virtual void Update(double elapsed) = 0;
};

#endif  /* VKTEST_GAMEOBJECT_H */
