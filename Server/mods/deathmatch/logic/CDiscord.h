/*****************************************************************************
 *
 *  PROJECT:     Multi Theft Auto v1.0
 *  LICENSE:     See LICENSE in the top level directory
 *  FILE:        mods/deathmatch/logic/CDiscord.h
 *  PURPOSE:     Discord bot manager
 *
 *  Multi Theft Auto is available from http://www.multitheftauto.com/
 *
 *****************************************************************************/

#pragma once

#ifdef _WIN32
    #include <dpp/win32_safe_warnings.h>
#endif
#include <dpp/dpp.h>

// Interfaces for modules

class IDiscordGuild : public dpp::guild
{
public:
    IDiscordGuild();
    ~IDiscordGuild();

    std::uint32_t         GetScriptID() const noexcept;
    static IDiscordGuild* GetFromSciptID(std::uint32_t id);

protected:
    std::uint32_t m_scriptID;
};

class IDiscord : public dpp::cluster
{
public:
    enum class DiscordEvent
    {
        on_voice_state_update,
        on_voice_client_disconnect,
        on_voice_client_speaking,
        on_log,
        on_guild_join_request_delete,
        on_interaction_create,
        on_slashcommand,
        on_button_click,
        on_autocomplete,
        on_select_click,
        on_message_context_menu,
        on_user_context_menu,
        on_form_submit,
        on_guild_delete,
        on_channel_delete,
        on_channel_update,
        on_ready,
        on_message_delete,
        on_guild_member_remove,
        on_resumed,
        on_guild_role_create,
        on_typing_start,
        on_message_reaction_add,
        on_guild_members_chunk,
        on_message_reaction_remove,
        on_guild_create,
        on_channel_create,
        on_message_reaction_remove_emoji,
        on_message_delete_bulk,
        on_guild_role_update,
        on_guild_role_delete,
        on_channel_pins_update,
        on_message_reaction_remove_all,
        on_voice_server_update,
        on_guild_emojis_update,
        on_guild_stickers_update,
        on_presence_update,
        on_webhooks_update,
        on_automod_rule_create,
        on_automod_rule_update,
        on_automod_rule_delete,
        on_automod_rule_execute,
        on_guild_member_add,
        on_invite_delete,
        on_guild_update,
        on_guild_integrations_update,
        on_guild_member_update,
        on_invite_create,
        on_message_update,
        on_user_update,
        on_message_create,
        on_message_poll_vote_add,
        on_message_poll_vote_remove,
        on_guild_audit_log_entry_create,
        on_guild_ban_add,
        on_guild_ban_remove,
        on_integration_create,
        on_integration_update,
        on_integration_delete,
        on_thread_create,
        on_thread_update,
        on_thread_delete,
        on_thread_list_sync,
        on_thread_member_update,
        on_thread_members_update,
        on_guild_scheduled_event_create,
        on_guild_scheduled_event_update,
        on_guild_scheduled_event_delete,
        on_guild_scheduled_event_user_add,
        on_guild_scheduled_event_user_remove,
        on_voice_buffer_send,
        on_voice_user_talking,
        on_voice_ready,
        on_voice_receive,
        on_voice_receive_combined,
        on_voice_track_marker,
        on_stage_instance_create,
        on_stage_instance_update,
        on_stage_instance_delete,
        on_entitlement_create,
        on_entitlement_update,
        on_entitlement_delete,
    };

public:
    IDiscord() noexcept;
    ~IDiscord() noexcept;

    bool HasStarted() const noexcept;

    void login(const std::string_view& token) noexcept;
    void start();
    void start(bool) = delete;
    void stop();

    template <typename F>
    void on(DiscordEvent event, F&& func);

    virtual IDiscordGuild* GetGuild(dpp::snowflake id) const noexcept = 0;

protected:
    std::atomic<bool> m_hasStarted;
    std::unordered_map<DiscordEvent, std::vector<bool>> m_eventHandlers;
};

// Usable classes

class CDiscordGuild : public IDiscordGuild
{
public:
    static CDiscordGuild* GetFromSciptID(std::uint32_t id);
};

class CDiscord : public IDiscord
{
public:
    CDiscord() noexcept;

    CDiscordGuild* GetGuild(dpp::snowflake id) const noexcept override;

protected:
};
