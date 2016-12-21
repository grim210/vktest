#ifndef VKTEST_GAMEOBJECT_H
#define VKTEST_GAMEOBJECT_H

/*
* This is a purely virtual class that encapsulates what it means to be a
* 'Game Object'.  That means any object that inherits this one can be assumed
* to be certain things.  Like drawable, updatable, rebuildable, etc.
*/

class GameObject {
public:
    virtual void Draw(void) = 0;
    virtual void Rebuild(void) = 0;
    virtual void Update(double elapsed) = 0;
};

#endif  /* VKTEST_GAMEOBJECT_H */
