#pragma
#ifndef __CIFPANIMATIONS_H
#define __CIFPANIMATIONS_H
#include "../Client/mods/deathmatch/logic/CClientIFP.h"

class CIFPAnimations
{
    public:

    struct SAnimation
    {
        SString                           Name;
        unsigned int                      u32NameHash;
        CAnimBlendHierarchySAInterface    Hierarchy;
        char *                            pSequencesMemory;
    };

	std::vector < SAnimation > vecAnimations;	

public:
            CIFPAnimations   ( void );
            ~CIFPAnimations  ( void );

    void    DeleteAnimations ( void );

};
#endif
