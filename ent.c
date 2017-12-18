#pragma once
#include "ent.h"
#include "simple_logger.h"
#include "gf2d_graphics.h"
#include "gf2d_sprite.h"


typedef struct
{
	Uint32 maxEntities;
	Entity *entityList;
	Uint64 autoincrement;

}EntityManager;

static EntityManager entity_manager = { 0 };

void gf2d_entity_system_close()
{
	int i;
	if (entity_manager.entityList != NULL)
	{
		for (i = 0; i < entity_manager.maxEntities; i++)
		{
			gf2d_entity_free(&entity_manager.entityList[i]);
		}
		free(entity_manager.entityList);
	}
	memset(&entity_manager, 0, sizeof(EntityManager));
	slog("entity system closed");
}

void gf2d_entity_system_init(Uint32 maxEntities)
{
	if (!maxEntities)
	{
		slog("cannot initialize entity system for zero entities");
		return;
	}
	memset(&entity_manager, 0, sizeof(EntityManager));

	entity_manager.entityList = (Entity*)malloc(sizeof(Entity)*maxEntities);
	if (!entity_manager.entityList)
	{
		slog("failed to allocate entity list");
		gf2d_entity_system_close();
		return;
	}
	memset(entity_manager.entityList, 0, sizeof(Entity)*maxEntities);
	entity_manager.maxEntities = maxEntities;
	atexit(gf2d_entity_system_close);
	slog("entity system initialized");
}

void gf2d_entity_free(Entity *self)
{
	int i;
	if (!self)return;
	if (self->free)self->free(self);
	/*for (i = 0; i < EntitySoundMax; i++)
	{
		gf2d_sound_free(self->sound[i]);
	}*/
	gf2d_actor_free(&self->actor);
	//gf2d_particle_emitter_free(self->pe);
	memset(self, 0, sizeof(Entity));
}

Entity *gf2d_entity_new()
{
	int i;
	for (i = 0; i < entity_manager.maxEntities; i++)
	{
		if (entity_manager.entityList[i].inuse == 0)
		{
			memset(&entity_manager.entityList[i], 0, sizeof(Entity));
			entity_manager.entityList[i].id = entity_manager.autoincrement++;
			entity_manager.entityList[i].inuse = 1;
			vector2d_set(entity_manager.entityList[i].scale, 1, 1);
			entity_manager.entityList[i].actor.color = vector4d(1, 1, 1, 1);// no color shift, opaque
			return &entity_manager.entityList[i];
		}
	}
	return NULL;
}

void gf2d_entity_draw(Entity *self)
{
	if (!self)return;
	if (!self->inuse)return;

	//gf2d_particle_emitter_draw(self->pe);

	gf2d_sprite_draw(
		self->sprite,
		self->position,
		&self->scale,
		&self->scaleCenter,
		NULL,
		NULL,
		NULL,
		(Uint32)self->frame);

	self->box.x = self->position.x; 
	self->box.y = self->position.y;
	self->box.w = self->sprite->frame_w;
	self->box.h = self->sprite->frame_h;
	Vector4D rectColor = { 255, 100, 255, 200 };

	gf2d_draw_rect(self->box, rectColor);


	/*if (self->draw != NULL)
	{
		self->draw(self);
	}*/
}

void gf2d_entity_draw_all()
{
	int i;
	for (i = 0; i < entity_manager.maxEntities; i++)
	{
		if (entity_manager.entityList[i].inuse == 0)continue;
		gf2d_entity_draw(&entity_manager.entityList[i]);
	}
}

void gf2d_entity_pre_sync_body(Entity *self)
{
	if (!self)return;// nothin to do
	vector2d_copy(self->body.velocity, self->velocity);
}

void gf2d_entity_post_sync_body(Entity *self)
{
	if (!self)return;// nothin to do
					 //    slog("entity %li : %s old position(%f,%f) => new position (%f,%f)",self->id,self->name,self->position,self->body.position);
	vector2d_copy(self->position, self->body.position);
}

void gf2d_entity_update(Entity *self)
{
	if (!self)return;
	if (!self->inuse)return;

	if (self->dead != 0)
	{
		gf2d_entity_free(self);
		return;
	}
	/*collision handles position and velocity*/
	vector2d_add(self->velocity, self->velocity, self->acceleration);

	//gf2d_particle_emitter_update(self->pe);

	gf2d_actor_next_frame(&self->actor);

	if (self->update != NULL)
	{
		self->update(self);
	}
}

void gf2d_entity_think_all()
{
	int i;
	for (i = 0; i < entity_manager.maxEntities; i++)
	{
		if (entity_manager.entityList[i].inuse == 0)continue;
		if (entity_manager.entityList[i].think != NULL)
		{
			entity_manager.entityList[i].think(&entity_manager.entityList[i]);
		}
	}
}

void gf2d_entity_pre_sync_all()
{
	int i;
	for (i = 0; i < entity_manager.maxEntities; i++)
	{
		if (entity_manager.entityList[i].inuse == 0)continue;
		gf2d_entity_pre_sync_body(&entity_manager.entityList[i]);
	}
}

void gf2d_entity_post_sync_all()
{
	int i;
	for (i = 0; i < entity_manager.maxEntities; i++)
	{
		if (entity_manager.entityList[i].inuse == 0)continue;
		gf2d_entity_post_sync_body(&entity_manager.entityList[i]);
	}
}

void gf2d_entity_update_all()
{
	int i;
	for (i = 0; i < entity_manager.maxEntities; i++)
	{
		if (entity_manager.entityList[i].inuse == 0)continue;
		gf2d_entity_update(&entity_manager.entityList[i]);
	}
}

/*collision check */
Bool collide(Entity *self, Entity *targ)
{
	if (!self->inuse) return false;

	if (self->box.x + self->box.w < targ->box.x || self->box.x > targ->box.x + targ->box.w ||
		self->box.y + self->box.h < targ->box.y || self->box.y > targ->box.y + targ->box.h)

			
		/*self->box.x < targ->box.x + targ->box.w && self->box.x + self->box.w > targ->box.x &&
		self->box.y < targ->box.y + targ->box.h && self->box.y + self->box.h > targ->box.y*/

	{
		return false; 
	}
	else { slog("Hit"); return true; } 

}

/*detection think*/
Bool detection(Entity *self, Entity *targ)
{
	if (!self->inuse) return false;
	
	float dx = self->position.x - targ->position.x;
	float dy = self->position.y - targ->position.y;
	float length = sqrt(dx * dx + dy * dy);
	//float currentTime = 0;
	//float lastTime = 0;
	//float dirx = 0;
	//float diry = 0;
	//float speed = 0.1;
	
	//float deltaTime = currentTime - lastTime;
	//deltaTime++;
	dx /= length; dy /= length; // normalize (make it 1 unit length)
	dx *= 1; dy *= 1; // scale to our desired speed

	if (length <= 150)
	{
		//targ->position.x += dirx * speed * deltaTime;
		//targ->position.y += diry * speed * deltaTime;

		targ->position.x += dx;
		targ->position.y += dy;
	}
	
}

/*spawn*/

/*
typedef struct
{
	char   *name;
	Entity *(*spawnFunction)(Vector2D position);
}SpawnPair;

Entity *spawn_entity(char *name, Vector2D position)
{
	SpawnPair *spawn;
	for (spawn = _spawnList; spawn->name != 0; spawn++)
	{
		if ((gf2d_line_cmp(spawn->name, name) == 0) && (spawn->spawnFunction != NULL))
		{
			return spawn->spawnFunction(position);
		}
	}
	slog("no spawn candidate found for %s", name);
	return NULL;
}
*/

void spawn_enemy(int x, int y, Entity *enemy)
{
	gf2d_entity_new();

	//enemy->sprite = sprite;
	enemy->inuse = 1;
	enemy->position = vector2d(x, y);
	enemy->frame = 0;
	enemy->scale = vector2d(0.5, 0.5);
	enemy->scaleCenter = vector2d(0, 0);

	//gf2d_entity_draw(&enemy);
}

/*int gf2d_entity_deal_damage(Entity *target, Entity *inflictor, Entity *attacker, int damage, Vector2D kick)
{
	Vector2D k;
	int inflicted;
	if (!target)return 0;
	if (!inflictor)return 0;
	if (!attacker)return 0;
	if (!target->damage)return 0;// cannot take damage
	if (!damage)return 0;// no damage to deal
	inflicted = target->damage(target, damage, inflictor);
	vector2d_scale(k, kick, (float)inflicted / (float)damage);
	vector2d_add(target->velocity, k, target->velocity);
	return inflicted;
}*/

/*eol@eof*/