using Microsoft.Maui.Controls;
using Microsoft.Maui.LifecycleEvents;
using Plugin.LocalNotification;
using Microsoft.Maui.ApplicationModel;

namespace SimpleMauiApp;

public static class MauiProgram
{
    public static MauiApp CreateMauiApp()
    {
        var builder = MauiApp.CreateBuilder();
        builder
            .UseMauiApp<App>()
            .UseLocalNotification()
            .ConfigureFonts(fonts =>
            {
                fonts.AddFont("OpenSans-Regular.ttf", "OpenSansRegular");
                fonts.AddFont("OpenSans-Semibold.ttf", "OpenSansSemibold");
            });

        builder.Services.AddSingleton<MainPage>();

#if ANDROID
        builder.ConfigureLifecycleEvents(events =>
        {
            events.AddAndroid(android => android
                .OnStart(async (activity) =>
                {
                    if (OperatingSystem.IsAndroidVersionAtLeast(33))
                    {
                        var status = await Permissions.CheckStatusAsync<Permissions.PostNotifications>();
                        if (status != PermissionStatus.Granted)
                        {
                            status = await Permissions.RequestAsync<Permissions.PostNotifications>();
                            if (status != PermissionStatus.Granted)
                            {
                                await MainThread.InvokeOnMainThreadAsync(async () =>
                                {
                                    await Application.Current.MainPage.DisplayAlert(
                                        "Permission Denied",
                                        "Notifications are disabled. Enable them in Settings.",
                                        "OK");
                                });
                            }
                        }
                    }
                }));
        });
#endif

        return builder.Build();
    }
}