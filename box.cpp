#include "box.h"

Box* Box::Init(VkDevice device, VkCommandPool pool, VkQueue queue)
{
    Box* box = new Box();

    box->m_vertices = {
       {{-0.5f, -0.5f,  0.25f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
       {{ 0.5f, -0.5f,  0.25f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
       {{ 0.5f,  0.5f,  0.25f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
       {{-0.5f,  0.5f,  0.25f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}},

       {{-0.5f, -0.5f, -0.25f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
       {{ 0.5f, -0.5f, -0.25f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
       {{ 0.5f,  0.5f, -0.25f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
       {{-0.5f,  0.5f, -0.25f}, {1.0f, 1.0f, 1.0f}, {0.0f, 1.0f}}
    };

    box->m_indices = {
        0, 1, 2, 2, 3, 0,
        4, 5, 6, 6, 7, 4
    };

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

