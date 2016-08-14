#include "box.h"

Box* Box::Init(void)
{
    Box* box = new Box();
    return box;
}

void Box::Release(Box* box)
{
    delete(box);
}

/* Inherited from GameObject */
void Box::Draw(void)
{

}

/* Inherited from GameObject */
void Box::Rebuild(void)
{

}

/* Inherited from GameObject */
void Box::Update(double elapsed)
{

}
