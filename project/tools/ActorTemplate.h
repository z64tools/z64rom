#ifndef __EN_ACTOR_H__
#define __EN_ACTOR_H__

#include <uLib.h>

struct EnActor;

typedef void (* EnActorFunc)(struct EnActor*, PlayState*);

typedef struct EnActor {
    Actor actor;
} EnActor;

#endif // __EN_ACTOR_H__