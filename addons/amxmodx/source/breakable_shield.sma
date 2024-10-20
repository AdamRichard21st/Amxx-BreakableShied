#include <amxmodx>
#include <reapi>
#include <fakemeta>
#include <engine>
#include <custom_entities_data>

// uncomment the directive below to enable cvar based damage
// #define CVAR_BASED_DAMAGE

#define getShieldHP(%0,%1)  CED_GetCell(%0, "shield-hp", %1)
#define setShieldHP(%0,%1)  CED_SetCell(%0, "shield-hp", %1)

public const SHIELD_GIB_MODEL[] = "models/metalplategibs.mdl"
public const SHIELD_GIB_SOUND[] = "debris/bustmetal1.wav"

public GibModelId
public CvarShieldHP

public KillShieldAfterDrop
public PlayerHadShieldLastTouch
public IsValidTouch

#if defined CVAR_BASED_DAMAGE
    public CvarShieldDamage
#else
    public LastDamage
#endif

public plugin_init()
{
    register_plugin("Breakable Shield", "1.0.0", "AdamRichard21st")

    bind_pcvar_num(register_cvar("breakable_shield_hp", "250"), CvarShieldHP)

    register_forward(FM_EmitSound, "OnEmitSound", ._post = 1)
    register_forward(FM_Touch, "OnTouchPre", ._post = 0)
    register_forward(FM_Touch, "OnTouch", ._post = 1)

    #if defined CVAR_BASED_DAMAGE
        bind_pcvar_num(register_cvar("breakable_shield_damage", "25"), CvarShieldDamage)
    #else
        RegisterHookChain(RG_CBaseEntity_FireBuckshots, "OnFireBuckshots", .post = 0)
        RegisterHookChain(RG_CBaseEntity_FireBullets3, "OnFireBullets3", .post = 0)
    #endif

    RegisterHookChain(RG_CBasePlayer_DropShield, "OnShieldDropped", .post = 1)
    RegisterHookChain(RG_CBasePlayer_GiveShield, "OnShieldAdded", .post = 1)
}

public plugin_end()
{
    CED_Free()
}

public plugin_precache()
{
    GibModelId = precache_model(SHIELD_GIB_MODEL)
    precache_sound(SHIELD_GIB_SOUND)
}

#if !defined CVAR_BASED_DAMAGE
    public OnFireBullets3(pEntity, Float:vecSrc[3], Float:vecDirShooting[3], Float:vecSpread, Float:flDistance, iPenetration, iBulletType, iDamage, Float:flRangeModifier, pevAttacker, bool:bPistol, shared_rand)
    {
        LastDamage = iDamage
    }

    public OnFireBuckshots(pEntity, cShots, Float:vecSrc[3], Float:vecDirShooting[3], Float:vecSpread[3], Float:flDistance, iTracerFreq, iDamage, pevAttacker)
    {
        LastDamage = iDamage
    }
#endif

public OnTouchPre(shield, id)
{
    if (!is_user_alive(id) || !FClassnameIs(shield, "weapon_shield"))
    {
        return
    }

    PlayerHadShieldLastTouch = get_member(id, m_bOwnsShield)
    IsValidTouch = 1
}

public OnTouch(shield, id)
{
    if (!IsValidTouch)
    {
        return
    }

    IsValidTouch = 0

    if (!PlayerHadShieldLastTouch && !get_member(id, m_bOwnsShield))
    {
        return
    }

    new hp
    
    getShieldHP(shield, hp)
    setShieldHP(id, hp)
}

public OnShieldAdded(id)
{
    setShieldHP(id, CvarShieldHP)
}

public OnShieldDropped(id)
{
    new hp, shield = GetHookChainReturn(ATYPE_INTEGER)

    if (KillShieldAfterDrop)
    {
        KillShieldAfterDrop = 0
        rg_remove_entity(shield)
        return
    }

    getShieldHP(id, hp)
    setShieldHP(shield, hp)
}

public OnEmitSound(id, channel, const sound[])
{
    if (!is_user_alive(id) || !equal(sound, "weapons/ric_metal-", 18))
    {
        return
    }

    OnShieldHitted(id)
}

OnShieldHitted(id)
{
    #if defined CVAR_BASED_DAMAGE
        setShieldDamage(id, CvarShieldDamage)
    #else
        setShieldDamage(id, LastDamage)
    #endif
}

setShieldDamage(id, damage)
{
    new currentShieldHP
    getShieldHP(id, currentShieldHP)

    new updatedShieldHP = clamp(currentShieldHP - damage, 0, CvarShieldHP)
    setShieldHP(id, updatedShieldHP)

    if (updatedShieldHP <= 0)
    {
        breakShield(id)
    }
}

breakShield(id)
{
    KillShieldAfterDrop = 1

    // a hacky way to remove a shield:
    // https://github.com/s1lentq/ReGameDLL_CS/blob/9b626b1d82f676287339240569934255613c4f0e/regamedll/dlls/player.cpp#L8187
    rg_drop_item(id, "")

    makeBreakEffects(id)
}

makeBreakEffects(id)
{
    new origin[3]
    get_user_origin(id, origin)

    emit_sound(id, CHAN_VOICE, SHIELD_GIB_SOUND, VOL_NORM, ATTN_NORM, SND_SPAWNING, PITCH_NORM)

    message_begin(MSG_BROADCAST, SVC_TEMPENTITY)
    {
        #define GIB_SIZE 16
        #define GIB_VELOCITY 50
        #define GIB_COUNT 10
        #define GIB_LIFE 25
        #define GIB_RANDOMINESS 10

        write_byte(TE_BREAKMODEL)
        write_coord(origin[0])
        write_coord(origin[1])
        write_coord(origin[2] + 24)
        write_coord(GIB_SIZE)
        write_coord(GIB_SIZE)
        write_coord(GIB_SIZE)
        write_coord(random_num(-GIB_VELOCITY,GIB_VELOCITY))
        write_coord(random_num(-GIB_VELOCITY,GIB_VELOCITY))
        write_coord(GIB_VELOCITY/2)
        write_byte(GIB_RANDOMINESS)
        write_short(GibModelId)
        write_byte(GIB_COUNT)
        write_byte(GIB_LIFE)
        write_byte(BREAK_METAL)
    }
    message_end()
}