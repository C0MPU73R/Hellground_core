/* 
 * Copyright (C) 2006-2008 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
 * Copyright (C) 2008-2013 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2008-2015 Hellground <http://hellground.net/>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

/* ScriptData
SDName: Boss_Grand_Warlock_Nethekurse
SD%Complete: 95
SDComment:
SDCategory: Hellfire Citadel, Shattered Halls
EndScriptData */

/* ContentData
boss_grand_warlock_nethekurse
mob_fel_orc_convert
EndContentData */

#include "precompiled.h"
#include "def_shattered_halls.h"

struct Say
{
    int32 id;
};

static Say PeonAttacked[]=
{
    {-1540001},
    {-1540002},
    {-1540003},
    {-1540004},
};
static Say PeonDies[]=
{
    {-1540005},
    {-1540006},
    {-1540007},
    {-1540008},
};

#define SAY_INTRO                   -1540000
#define SAY_TAUNT_1                 -1540009
#define SAY_TAUNT_2                 -1540010
#define SAY_TAUNT_3                 -1540011
#define SAY_AGGRO_1                 -1540012
#define SAY_AGGRO_2                 -1540013
#define SAY_AGGRO_3                 -1540014
#define SAY_SLAY_1                  -1540015
#define SAY_SLAY_2                  -1540016
#define SAY_DIE                     -1540017

#define SPELL_DEATH_COIL            30500
#define SPELL_DARK_SPIN             30502
#define SPELL_SHADOW_FISSURE        30496
#define SPELL_SHADOW_SEAR           30735
#define SPELL_SHADOW_BOLT           30505
#define SPELL_DARK_CLEAVE           30508
#define SPELL_SHADOW_CLEAVE         30495
#define H_SPELL_SHADOW_SLAM         35953

#define SPELL_HEMORRHAGE            30478
#define SPELL_CONSUMPTION           (HeroicMode ? 35952 : 30497)
#define NPC_FEL_ORC                 17083

struct boss_grand_warlock_nethekurseAI : public ScriptedAI
{
    boss_grand_warlock_nethekurseAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = c->GetInstanceData();
    }

    ScriptedInstance* pInstance;
    bool IntroOnce;
    bool IsIntroEvent;
    bool SpinOnce;
    bool Phase;

    uint32 PeonEngagedCount;
    uint32 PeonKilledCount;
    std::list<uint64 > orcs;

    Timer IntroEvent_Timer;
    Timer DeathCoil_Timer;
    Timer ShadowFissure_Timer;
    Timer Cleave_Timer;
    Timer ShadowBolt_Timer;
    Timer DarkCleave_Timer;

    void Reset()
    {
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SPAWNING);
        IsIntroEvent = false;
        SpinOnce = false;
        Phase = false;

        PeonEngagedCount = 0;
        PeonKilledCount = 0;

        IntroEvent_Timer.Reset(0);
        DeathCoil_Timer.Reset(20000);
        ShadowFissure_Timer.Reset(8000);
        Cleave_Timer.Reset(17000);
        ShadowBolt_Timer.Reset(1000);
        DarkCleave_Timer.Reset(1000);

        if (pInstance)
            pInstance->SetData(TYPE_NETHEKURSE, NOT_STARTED);
        orcs = me->GetMap()->GetCreaturesGUIDList(NPC_FEL_ORC);
    }

    void DoYellForPeonEnterCombat()
    {
        if (PeonEngagedCount >= 4)
            return;

        DoScriptText(PeonAttacked[PeonEngagedCount].id, me);
        ++PeonEngagedCount;
    }

    void DoYellForPeonDeath()
    {
        if (PeonKilledCount >= 4)
            return;

        DoScriptText(PeonDies[PeonKilledCount].id, me);
        ++PeonKilledCount;

        if (PeonKilledCount == 3)
        {
            me->CastSpell((Unit*)NULL, SPELL_SHADOW_SEAR, true);
            IntroEvent_Timer.Reset(20000);
        }
        if (PeonKilledCount == 4)
        {
            DoScriptText(RAND(SAY_TAUNT_1, SAY_TAUNT_2, SAY_TAUNT_3), me);
            IsIntroEvent = false;
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SPAWNING);
            DoZoneInCombat();
        }
    }

    void AttackStart(Unit* who)
    {
        if (IsIntroEvent)
            return;

        ScriptedAI::AttackStart(who);
    }

    void MoveInLineOfSight(Unit *who)
    {
        if (me->IsWithinDistInMap(who, 40.0f) && pInstance && pInstance->GetData(TYPE_NETHEKURSE) == NOT_STARTED)
        {
            if (who->GetTypeId() != TYPEID_PLAYER || ((Player*)who)->IsGameMaster())
                return;

            DoScriptText(SAY_INTRO, me);
            IsIntroEvent = true;

            pInstance->SetData(TYPE_NETHEKURSE,IN_PROGRESS);
        }

        if (IsIntroEvent)
            return;

        ScriptedAI::MoveInLineOfSight(who);
    }

    void EnterCombat(Unit* who)
    {
        DoScriptText(RAND(SAY_AGGRO_1, SAY_AGGRO_2, SAY_AGGRO_3), me);
    }

    void JustSummoned(Creature *summoned)
    {
        summoned->setFaction(14);
        summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SPAWNING);
        summoned->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE);
        summoned->CastSpell(summoned,SPELL_CONSUMPTION,false,0,0,me->GetGUID());
    }

    void KilledUnit(Unit* victim)
    {
        DoScriptText(RAND(SAY_SLAY_1, SAY_SLAY_2), me);
    }

    void EnterEvadeMode()
    {
        me->RemoveAllAuras();
        me->DeleteThreatList();
        me->CombatStop(true);
        me->GetUnitStateMgr().InitDefaults(true);
        
        if (!me->IsAlive())
            return;    

        if (pInstance)
        {
            for (std::list<uint64>::iterator itr = orcs.begin(); itr != orcs.end(); itr++)
            {
                Creature* Orc = me->GetCreature(*itr);
                if (!Orc || Orc->IsAlive())
                    continue;

                Orc->Respawn();
            }
        }
    }

    void JustDied(Unit* killer)
    {
        DoScriptText(SAY_DIE, me);

        if (!pInstance)
            return;

        pInstance->SetData(TYPE_NETHEKURSE,DONE);
    }

    void UpdateAI(const uint32 diff)
    {
        if (IsIntroEvent)
        {
            if (IntroEvent_Timer.Expired(diff))
            {
                for (std::list<uint64>::iterator itr = orcs.begin(); itr != orcs.end(); itr++)
                {
                    Creature* Orc = me->GetCreature(*itr);
                    if (!Orc || !Orc->IsAlive())
                        continue;

                    me->Kill(Orc);
                }
                IntroEvent_Timer = 0;
            }
            return;
        }
        if (!UpdateVictim())
            return;

        if (Phase)
        {
            if (!SpinOnce)
            {
                me->GetUnitStateMgr().PushAction(UNIT_ACTION_ROOT);
                DoCast(me, SPELL_DARK_SPIN);
                SpinOnce = true;
            }

            if (ShadowBolt_Timer.Expired(diff))
            {
                if (Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM,0))
                    DoCast(pTarget,SPELL_SHADOW_BOLT);
                ShadowBolt_Timer = 1000;
            }

            if (DarkCleave_Timer.Expired(diff))
            {
                DoCast(me->GetVictim(),SPELL_DARK_CLEAVE);
                DarkCleave_Timer = 1000;
            }
        }
        else
        {
            if (ShadowFissure_Timer.Expired(diff))
            {
                if (Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM,0))
                    DoCast(pTarget,SPELL_SHADOW_FISSURE);
                ShadowFissure_Timer = 7500+rand()%7500;
            }

            if (DeathCoil_Timer.Expired(diff))
            {
                if (Unit *pTarget = SelectUnit(SELECT_TARGET_RANDOM,0))
                    DoCast(pTarget,SPELL_DEATH_COIL);
                DeathCoil_Timer = 15000+rand()%5000;
            }

            if (Cleave_Timer.Expired(diff))
            {
                DoCast(me->GetVictim(),(HeroicMode ? H_SPELL_SHADOW_SLAM : SPELL_SHADOW_CLEAVE));
                Cleave_Timer = 20000+rand()%2500;
            }

            if ((me->GetHealth()*100) / me->GetMaxHealth() <= 20)
                Phase = true;

            DoMeleeAttackIfReady();
        }
    }
};

struct mob_fel_orc_convertAI : public ScriptedAI
{
    mob_fel_orc_convertAI(Creature *c) : ScriptedAI(c)
    {
        pInstance = c->GetInstanceData();
    }

    ScriptedInstance* pInstance;
    Timer Hemorrhage_Timer;
    Timer Kill_Timer;

    void Reset()
    {
        me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SPAWNING);
        me->SetNoCallAssistance(true);
        Hemorrhage_Timer.Reset(3000);
        Kill_Timer = 0;
    }

    void MoveInLineOfSight(Unit* who)
    {
        // dont allow puling by sending pet
        if (who->GetTypeId() == TYPEID_PLAYER && me->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SPAWNING) &&
            who->IsWithinDist(me,10.0))
            me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_SPAWNING);

        // do nothing mote
        return;
    }

    void EnterCombat(Unit* who)
    {
        if (pInstance)
        {
            if (pInstance->GetData64(DATA_NETHEKURSE))
            {
                Creature *pKurse = Unit::GetCreature(*me,pInstance->GetData64(DATA_NETHEKURSE));
                if (pKurse)
                    ((boss_grand_warlock_nethekurseAI*)pKurse->AI())->DoYellForPeonEnterCombat();
            }

            if (pInstance->GetData(TYPE_NETHEKURSE) == IN_PROGRESS)
                return;
            else pInstance->SetData(TYPE_NETHEKURSE,IN_PROGRESS);
        }
    }

    void SpellHit(Unit* caster, const SpellEntry* spell)
    {
        if (spell->Id == SPELL_SHADOW_SEAR)
        {
            if (pInstance->GetData(TYPE_NETHEKURSE) == IN_PROGRESS)
                Kill_Timer = 20000;
        }
    }

    void JustDied(Unit* killer)
    {
        if (pInstance)
        {
            if (pInstance->GetData64(DATA_NETHEKURSE))
            {
                Creature *pKurse = Unit::GetCreature(*me,pInstance->GetData64(DATA_NETHEKURSE));
                if (pKurse)
                    ((boss_grand_warlock_nethekurseAI*)pKurse->AI())->DoYellForPeonDeath();
            }
        }
    }

    void UpdateAI(const uint32 diff)
    {
        if (Kill_Timer.Expired(diff))
        {
            me->DealDamage(me, me->GetHealth(), DIRECT_DAMAGE, SPELL_SCHOOL_MASK_NORMAL, NULL, false);
            Kill_Timer = 0;
        }

        if (!UpdateVictim())
            return;

        if (Hemorrhage_Timer.Expired(diff))
        {
            DoCast(me->GetVictim(),SPELL_HEMORRHAGE);
            Hemorrhage_Timer = 15000;
        }

        DoMeleeAttackIfReady();
    }
};

CreatureAI* GetAI_boss_grand_warlock_nethekurse(Creature* creature)
{
    return new boss_grand_warlock_nethekurseAI (creature);
}

CreatureAI* GetAI_mob_fel_orc_convert(Creature* creature)
{
    return new mob_fel_orc_convertAI (creature);
}

void AddSC_boss_grand_warlock_nethekurse()
{
    Script *newscript;

    newscript = new Script;
    newscript->Name = "boss_grand_warlock_nethekurse";
    newscript->GetAI = &GetAI_boss_grand_warlock_nethekurse;
    newscript->RegisterSelf();

    newscript = new Script;
    newscript->Name = "mob_fel_orc_convert";
    newscript->GetAI = &GetAI_mob_fel_orc_convert;
    newscript->RegisterSelf();

}

