#include <uLib.h>
#include <code/z_lights.h>
#include <code/z_effect.h>

s32 Lights_SortLightList(PlayState* play, LightInfo* sortedLightList[7]) {
    LightNode* listHead = play->lightCtx.listHead;
    LightInfo* lights[32] = { NULL };
    s32 lightCount = 0;
    s32 sortedCount = 0;
    s32 id = 0;
    float smallestDist;
    
    if (listHead == NULL || listHead->info == NULL)
        return 0;
    
    // Get pointlights to list
    while (listHead->next != NULL) {
        if (listHead->info->type != LIGHT_DIRECTIONAL) {
            lights[lightCount] = listHead->info;
            lightCount++;
        }
        listHead = listHead->next;
    }
    
    for (s32 lightSlot = 0; lightSlot < 5; lightSlot++) {
        if (lightSlot >= lightCount)
            break;
        smallestDist = 900000000000000.0f;
        for (s32 i = 0; i < lightCount; i++) {
            if (lights[i] != NULL && lights[i]->params.point.radius > 0) {
                Vec3f pos = {
                    lights[i]->params.point.x,
                    lights[i]->params.point.y,
                    lights[i]->params.point.z
                };
                Camera* cam = GET_ACTIVE_CAM(play);
                float dist = Math_Vec3f_DistXYZ(&cam->eyeNext, &pos);
                
                if (dist < smallestDist) {
                    smallestDist = dist;
                    id = i;
                }
            }
        }
        if (lights[id] == NULL)
            break;
        sortedLightList[lightSlot] = lights[id];
        sortedCount++;
        lights[id] = NULL;
    }
    
    listHead = play->lightCtx.listHead;
    
    if (sortedCount < 7) {
        // Get dir lights to list
        while (sortedCount < 7) {
            if (listHead == NULL)
                break;
            if (listHead->info->type == LIGHT_DIRECTIONAL) {
                sortedLightList[sortedCount] = listHead->info;
                sortedCount++;
            }
            listHead = listHead->next;
        }
    }
    
    return sortedCount;
}

void Lights_SetPointlight(PlayState* play, Lights* lights, LightParams* params, bool isWiiVC) {
    float radiusF = (float)params->point.radius / 255;
    
    Light* light = Lights_FindSlot(lights);
    
    if (light == NULL)
        return;
    
    light->lPos.pos[0] = params->point.x;
    light->lPos.pos[1] = params->point.y;
    light->lPos.pos[2] = params->point.z;
    
    if (isWiiVC == 1) {
        light->lPos.col[0] = params->point.color[0];
        light->lPos.col[1] = params->point.color[1];
        light->lPos.col[2] = params->point.color[2];
        
        radiusF = 128 - 128 * (radiusF * radiusF);
        // radiusF /= 2.01;
        if (radiusF < 10)
            radiusF = 10;
        if (radiusF > 255)
            radiusF = 255;
        light->lPos.pad1 = 1;
        light->lPos.pad2 = radiusF;
        light->lPos.pad3 = 0x80;
    } else {
        light->lPos.col[0] = light->lPos.colc[0] = params->point.color[0];
        light->lPos.col[1] = light->lPos.colc[1] = params->point.color[1];
        light->lPos.col[2] = light->lPos.colc[2] = params->point.color[2];
        
        radiusF = 255 - 255 * (radiusF * radiusF);
        if (radiusF < 10)
            radiusF = 10;
        if (radiusF > 255)
            radiusF = 255;
        // light->lPos.pad1 = 0x1;
        // light->lPos.pad2 = 0xFF;
        light->lPos.pad1 = 1;
        light->lPos.pad2 = 1;
        light->lPos.pad3 = radiusF;
    }
}

void Lights_RebindActor(PlayState* play, Actor* actor, Vec3f* bindPos) {
    Lights* lights;
    
    if (!(actor->flags & ACTOR_FLAG_22))
        actor->flags |= ACTOR_FLAG_22;
    lights = LightContext_NewLights(&play->lightCtx, play->state.gfxCtx);
    Lights_BindAll(lights, play->lightCtx.listHead, bindPos);
    Lights_Draw(lights, play->state.gfxCtx);
}

void Lights_RebindPointlightsActor(PlayState* play, Actor* actor, bool isWiiVC) {
    Lights* lights;
    LightInfo* sortedList[7] = { NULL };
    
    if (!(actor->flags & ACTOR_FLAG_22))
        actor->flags |= ACTOR_FLAG_22;
    
    lights = LightContext_NewLights(&play->lightCtx, play->state.gfxCtx);
    s32 lightCount = Lights_SortLightList(play, sortedList);
    
    if (lightCount <= 0)
        return;
    
    for (s32 i = 0; i < lightCount; i++) {
        if (sortedList[i] == NULL)
            break;
        LightInfo* info = sortedList[i];
        LightParams* params = &info->params;
        
        if (info->type != LIGHT_DIRECTIONAL) {
            float radiusF = (float)params->point.radius / 255;
            
            Light* light = Lights_FindSlot(lights);
            
            if (light == NULL)
                return;
            
            light->lPos.pos[0] = params->point.x * 100.0f;
            light->lPos.pos[1] = params->point.y * 100.0f;
            light->lPos.pos[2] = params->point.z * 100.0f;
            
            if (isWiiVC) {
                light->lPos.col[0] = params->point.color[0];
                light->lPos.col[1] = params->point.color[1];
                light->lPos.col[2] = params->point.color[2];
                
                radiusF = 128 - 128 * (radiusF * radiusF);
                if (radiusF < 10)
                    radiusF = 10;
                if (radiusF > 255)
                    radiusF = 255;
                light->lPos.pad1 = 1;
                light->lPos.pad2 = radiusF;
                light->lPos.pad3 = 0x80;
            } else {
                light->lPos.col[0] = light->lPos.colc[0] = params->point.color[0];
                light->lPos.col[1] = light->lPos.colc[1] = params->point.color[1];
                light->lPos.col[2] = light->lPos.colc[2] = params->point.color[2];
                
                radiusF = 255 - 255 * (radiusF * radiusF);
                if (radiusF < 10)
                    radiusF = 10;
                if (radiusF > 255)
                    radiusF = 255;
                // light->lPos.pad1 = 0x1;
                // light->lPos.pad2 = 0xFF;
                light->lPos.pad1 = 1;
                light->lPos.pad2 = 1;
                light->lPos.pad3 = radiusF;
            }
        } else if (info->type == LIGHT_DIRECTIONAL) {
            Light* light = Lights_FindSlot(lights);
            if (light == NULL)
                return;
            
            light->l.col[0] = light->l.colc[0] = params->dir.color[0];
            light->l.col[1] = light->l.colc[1] = params->dir.color[1];
            light->l.col[2] = light->l.colc[2] = params->dir.color[2];
            light->l.dir[0] = params->dir.x;
            light->l.dir[1] = params->dir.y;
            light->l.dir[2] = params->dir.z;
            light->l.pad1 = 0;
        }
    }
    
    Lights_Draw(lights, play->state.gfxCtx);
}
