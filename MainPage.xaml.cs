using System;
using System.Net.Http;
using System.Text.Json;
using System.Threading.Tasks;
using Microsoft.Maui.Controls;
using Plugin.LocalNotification;

namespace SimpleMauiApp;

public partial class MainPage : ContentPage
{
    private string esp32IP = "http://192.168.43.230"; // Update with ESP32's IP from Serial Monitor
    private bool isPolling = false;
    private int notificationCount = 0;
    private const int maxRetries = 3;

    public MainPage()
    {
        try
        {
            InitializeComponent();
            if (Plant1MoistureLabel == null || Plant2MoistureLabel == null || Plant3MoistureLabel == null || StatusLabel == null ||
                Plant1Button == null || Plant2Button == null || Plant3Button == null || IpEntry == null)
            {
                System.Diagnostics.Debug.WriteLine("Error: One or more XAML elements are null after InitializeComponent.");
                throw new InvalidOperationException("XAML elements not initialized properly.");
            }
            Plant1MoistureLabel.Text = "Plant 1 Moisture: Waiting for data...";
            Plant2MoistureLabel.Text = "Plant 2 Moisture: Waiting for data...";
            Plant3MoistureLabel.Text = "Plant 3 Moisture: Waiting for data...";
            StatusLabel.Text = "Status: Waiting...";
            System.Diagnostics.Debug.WriteLine("MainPage initialized successfully");
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"InitializeComponent failed: {ex.Message}");
            if (Plant1MoistureLabel != null)
                Plant1MoistureLabel.Text = $"Error: {ex.Message}";
        }
    }

    private async void OnUpdateIpClicked(object sender, EventArgs e)
    {
        if (IpEntry != null && !string.IsNullOrWhiteSpace(IpEntry.Text))
        {
            esp32IP = IpEntry.Text.Trim();
            if (!esp32IP.StartsWith("http://"))
            {
                esp32IP = $"http://{esp32IP}";
            }
            await MainThread.InvokeOnMainThreadAsync(() =>
            {
                Plant1MoistureLabel.Text = $"IP updated to {esp32IP}. Test network to start.";
            });
            System.Diagnostics.Debug.WriteLine($"IP updated to: {esp32IP}");
        }
        else
        {
            await MainThread.InvokeOnMainThreadAsync(() =>
            {
                Plant1MoistureLabel.Text = "Error: Enter a valid IP address";
            });
        }
    }

    private async void OnTestNetworkClicked(object sender, EventArgs e)
    {
        string url = $"{esp32IP}/status";
        bool success = await SendHttpRequestAsync(url, "Testing network connection...");
        await MainThread.InvokeOnMainThreadAsync(() =>
        {
            Plant1MoistureLabel.Text = success ? "Network test successful! Polling started." : $"Network test failed: Check ESP32 connection";
        });
        if (success && !isPolling)
        {
            StartMoisturePolling();
        }
    }

    private async Task<bool> SendHttpRequestAsync(string url, string context, int retries = 0)
    {
        try
        {
            using var client = new HttpClient { Timeout = TimeSpan.FromSeconds(15) };
            HttpResponseMessage response = await client.GetAsync(url);
            response.EnsureSuccessStatusCode();
            return true;
        }
        catch (Exception ex)
        {
            if (retries < maxRetries)
            {
                await Task.Delay(1000);
                return await SendHttpRequestAsync(url, context, retries + 1);
            }
            await MainThread.InvokeOnMainThreadAsync(() =>
            {
                Plant1MoistureLabel.Text = $"{context} Error: {ex.Message} (URL: {url})";
            });
            System.Diagnostics.Debug.WriteLine($"{context} Error: {ex.Message} (URL: {url})");
            return false;
        }
    }

    private async void StartMoisturePolling()
    {
        if (isPolling) return;
        isPolling = true;

        while (isPolling)
        {
            string url = $"{esp32IP}/status";
            try
            {
                using var client = new HttpClient { Timeout = TimeSpan.FromSeconds(15) };
                HttpResponseMessage response = await client.GetAsync(url);
                response.EnsureSuccessStatusCode();
                string responseBody = await response.Content.ReadAsStringAsync();

                using var jsonDoc = JsonDocument.Parse(responseBody);
                var root = jsonDoc.RootElement;
                var moistureArray = root.GetProperty("moisture").EnumerateArray();
                int[] moistures = new int[3];
                int index = 0;
                foreach (var item in moistureArray)
                {
                    moistures[index++] = item.GetInt32();
                }
                bool pumpRunning = root.GetProperty("pumpRunning").GetBoolean();
                var valvesArray = root.GetProperty("valvesRunning").EnumerateArray();
                bool[] valvesRunning = new bool[3];
                index = 0;
                foreach (var item in valvesArray)
                {
                    valvesRunning[index++] = item.GetBoolean();
                }

                await MainThread.InvokeOnMainThreadAsync(() =>
                {
                    Plant1MoistureLabel.Text = $"Plant 1 Moisture: {moistures[0]}%";
                    Plant2MoistureLabel.Text = $"Plant 2 Moisture: {moistures[1]}%";
                    Plant3MoistureLabel.Text = $"Plant 3 Moisture: {moistures[2]}%";
                    StatusLabel.Text = $"Valves: {valvesRunning[0]} | {valvesRunning[1]} | {valvesRunning[2]} , Pump: {pumpRunning}";
                });

                // Send separate notifications for each plant
                for (int i = 0; i < 3; i++)
                {
                    try
                    {
                        var notification = new NotificationRequest
                        {
                            NotificationId = 1000 + notificationCount++,
                            Title = $"Plant {i + 1} Moisture Update",
                            Description = $"Moisture: {moistures[i]}%",
                            BadgeNumber = notificationCount,
                            CategoryType = NotificationCategoryType.Status
                        };
                        await LocalNotificationCenter.Current.Show(notification);
                    }
                    catch (Exception ex)
                    {
                        System.Diagnostics.Debug.WriteLine($"Notification error for Plant {i + 1}: {ex.Message}");
                    }
                }
            }
            catch (Exception ex)
            {
                await MainThread.InvokeOnMainThreadAsync(() =>
                {
                    Plant1MoistureLabel.Text = $"Polling error: {ex.Message} (URL: {url})";
                });
                System.Diagnostics.Debug.WriteLine($"Polling error: {ex.Message} (URL: {url})");
            }
            await Task.Delay(20000); // Match ESP32's 20s interval
        }
    }

    private async void OnPlant1Pressed(object sender, EventArgs e)
    {
        string url = $"{esp32IP}/valve1on";
        if (await SendHttpRequestAsync(url, "Plant 1 ON"))
        {
            await MainThread.InvokeOnMainThreadAsync(() =>
            {
                Plant1Button.Text = "Watering Plant 1";
            });
        }
    }

    private async void OnPlant1Released(object sender, EventArgs e)
    {
        string url = $"{esp32IP}/valve1off";
        if (await SendHttpRequestAsync(url, "Plant 1 OFF"))
        {
            await MainThread.InvokeOnMainThreadAsync(() =>
            {
                Plant1Button.Text = "Hold to Water Plant 1";
            });
        }
    }

    private async void OnPlant2Pressed(object sender, EventArgs e)
    {
        string url = $"{esp32IP}/valve2on";
        if (await SendHttpRequestAsync(url, "Plant 2 ON"))
        {
            await MainThread.InvokeOnMainThreadAsync(() =>
            {
                Plant2Button.Text = "Watering Plant 2";
            });
        }
    }

    private async void OnPlant2Released(object sender, EventArgs e)
    {
        string url = $"{esp32IP}/valve2off";
        if (await SendHttpRequestAsync(url, "Plant 2 OFF"))
        {
            await MainThread.InvokeOnMainThreadAsync(() =>
            {
                Plant2Button.Text = "Hold to Water Plant 2";
            });
        }
    }

    private async void OnPlant3Pressed(object sender, EventArgs e)
    {
        string url = $"{esp32IP}/valve3on";
        if (await SendHttpRequestAsync(url, "Plant 3 ON"))
        {
            await MainThread.InvokeOnMainThreadAsync(() =>
            {
                Plant3Button.Text = "Watering Plant 3";
            });
        }
    }

    private async void OnPlant3Released(object sender, EventArgs e)
    {
        string url = $"{esp32IP}/valve3off";
        if (await SendHttpRequestAsync(url, "Plant 3 OFF"))
        {
            await MainThread.InvokeOnMainThreadAsync(() =>
            {
                Plant3Button.Text = "Hold to Water Plant 3";
            });
        }
    }

    private async void OnTestNotificationClicked(object sender, EventArgs e)
    {
        try
        {
            var notification = new NotificationRequest
            {
                NotificationId = 1000 + notificationCount++,
                Title = "Test Notification",
                Description = "This is a test notification",
                BadgeNumber = notificationCount,
                CategoryType = NotificationCategoryType.Status
            };
            await LocalNotificationCenter.Current.Show(notification);
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Test notification error: {ex.Message}");
            await DisplayAlert("Error", $"Notification failed: {ex.Message}", "OK");
        }
    }

    protected override void OnDisappearing()
    {
        isPolling = false;
        base.OnDisappearing();
    }
}