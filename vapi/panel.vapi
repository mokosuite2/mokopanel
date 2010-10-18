namespace MokoPanel {
    [CCode (cheader_filename = "panel.h")]

    [CCode (cname="mokopanel_notification_queue")]
    public uint notification_queue(void* panel_ptr,
        string? app_name, uint id, string? icon, string summary, string? body,
        string[] actions, void* hints, int timeout);

    [CCode (cname="mokopanel_notification_remove")]
    public void notification_remove(void* panel_ptr, uint id);

    [CCode (cname="mokopanel_notification_caps")]
    public string[] notification_caps(void* panel_ptr);
}
