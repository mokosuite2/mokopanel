/**
 * Implementazione di un servizio di notifiche per mokopanel
 */

[DBus (name = "org.freedesktop.Notifications")]
public class NotificationsService : Object
{
    private void* panel;

    public NotificationsService(void* panel_ptr)
    {
        try {
            this.panel = panel_ptr;
            var conn = DBus.Bus.get (DBus.BusType.SESSION);
            dynamic DBus.Object bus = conn.get_object ("org.freedesktop.DBus",
                                                    "/org/freedesktop/DBus",
                                                    "org.freedesktop.DBus");

            // try to register service in session bus
            uint reply = bus.request_name ("org.freedesktop.Notifications", (uint) 0);
            assert (reply == DBus.RequestNameReply.PRIMARY_OWNER);

            conn.register_object ("/org/freedesktop/Notifications", this);
        }

        catch (DBus.Error e) {
            error ("%s", e.message);
        }
    }

    public uint Notify(string? app_name, uint id, string? icon,
            string summary, string? body, string[] actions,
            HashTable<string,Value?> hints, int timeout)
    {
        return MokoPanel.notification_queue(this.panel, app_name, id, icon,
            summary, body, actions, hints, timeout);
    }

    public void CloseNotification(uint id)
    {
        MokoPanel.notification_remove(this.panel, id);
    }

    public string[] GetCapabilities()
    {
        return MokoPanel.notification_caps(this.panel);
    }

    public void GetServerInformation(out string name, out string vendor, out string version, out string spec_version)
    {
        name = Config.PACKAGE_NAME;
        vendor = "Mokosuite";
        version = Config.VERSION;
        spec_version = "0.9";
    }
}
