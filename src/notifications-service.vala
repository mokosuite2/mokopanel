/**
 * Implementazione di un servizio di notifiche per mokopanel
 */

using MokoPanel;

[DBus (name = "org.freedesktop.Notifications")]
public class NotificationsService : Object
{
    private void* panel;

    private void on_bus_aquired (DBusConnection conn) {
        try {
            conn.register_object ("/org/freedesktop/Notifications", this);
        } catch (IOError e) {
            critical("Could not register service\n");
        }
    }

    public NotificationsService(void* panel_ptr)
    {
        this.panel = panel_ptr;
        Bus.own_name (BusType.SESSION, "org.freedesktop.Notifications", BusNameOwnerFlags.NONE,
                    this.on_bus_aquired,
                    () => {},
                    () => critical("Could not aquire D-Bus name"));

    }

    /* === D-BUS API === */

    public uint Notify(string? app_name, uint id, string? icon,
            string summary, string? body, string[] actions,
            HashTable<string,Variant> hints, int timeout)
    {
        return MokoPanel.notification_queue(this.panel, app_name, id, icon,
            summary, body, actions, actions.length,
            hints, timeout);
    }
}
