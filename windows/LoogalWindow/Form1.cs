using System.Diagnostics;
using System.Text.Json;
using Microsoft.Win32;

namespace LoogalWindow;

public class ScheduleItem
{
    public string Folder { get; set; } = "";
    public string Time { get; set; } = "";
    public string LastRunDate { get; set; } = "";
}

public partial class Form1 : Form
{
    private PictureBox previewBox = null!;
    private FlowLayoutPanel resultsPanel = null!;
    private Label statusLabel = null!;
    private Label selectedDirsLabel = null!;
    private ProgressBar progressBar = null!;
    private TextBox timeBox = null!;

    private readonly List<string> selectedDirs = new();
    private readonly List<ScheduleItem> schedules = new();
    private string scheduledDir = "";

    private string AppDir => AppContext.BaseDirectory;
    private string LoogalExe => Path.Combine(AppDir, "loogal.exe");
    private string ScheduleFile => Path.Combine(AppDir, "loogal-schedules.json");
    private string DebugFile => Path.Combine(AppDir, "loogal-gui-debug.txt");

    public Form1()
    {
        InitializeComponent();

        Directory.SetCurrentDirectory(AppDir);
        Directory.CreateDirectory(Path.Combine(AppDir, "data"));
        Directory.CreateDirectory(Path.Combine(AppDir, "data", "logs"));
        Directory.CreateDirectory(Path.Combine(AppDir, "manifests"));

        Text = "Loogal";
        Width = 1180;
        Height = 860;
        BackColor = Color.FromArgb(245, 245, 245);
        AllowDrop = true;
        KeyPreview = true;

        LoadSchedules();
        BuildUi();
        StartScheduleTimer();
    }

    private static string SafeString(JsonElement el, string name)
        => el.TryGetProperty(name, out var v) ? v.GetString() ?? "" : "";

    private static int SafeInt(JsonElement el, string name)
        => el.TryGetProperty(name, out var v) && v.ValueKind == JsonValueKind.Number ? v.GetInt32() : 0;

    private static double SafeDouble(JsonElement el, string name)
        => el.TryGetProperty(name, out var v) && v.ValueKind == JsonValueKind.Number ? v.GetDouble() : 0;

    private void LogDebug(string text)
    {
        try
        {
            File.AppendAllText(DebugFile, $"[{DateTime.Now:yyyy-MM-dd HH:mm:ss}] {text}{Environment.NewLine}");
        }
        catch { }
    }

    private void BuildUi()
    {
        var intro = new Label
        {
            Top = 20,
            Left = 20,
            Width = 1120,
            Height = 80,
            Text =
                "Start by indexing folders you want Loogal to remember.\n\n" +
                "Then search instantly: drag & drop local images, or press Ctrl+V for copied images or image links.\n\n" +
                "Loogal recognizes images by appearance - independent of name or location.",
            Font = new Font("Segoe UI Semibold", 10),
            ForeColor = Color.FromArgb(65, 65, 65)
        };

        var pickBtn = new Button { Text = "Pick Directories", Top = 115, Left = 20, Width = 200, Height = 40 };
        var indexBtn = new Button { Text = "Index Selected", Top = 185, Left = 20, Width = 200, Height = 40 };

        var pickHelp = new Label
        {
            Top = 158,
            Left = 25,
            Width = 360,
            Height = 22,
            Text = "Choose one or more folders Loogal should remember.",
            Font = new Font("Segoe UI", 8),
            ForeColor = Color.Gray
        };

        selectedDirsLabel = new Label
        {
            Top = 228,
            Left = 20,
            Width = 360,
            Height = 24,
            Text = "No folders selected.",
            Font = new Font("Segoe UI", 8),
            ForeColor = Color.DimGray
        };

        var schedulePickBtn = new Button { Text = "Pick Directory to Schedule", Top = 115, Left = 440, Width = 230, Height = 40 };
        var scheduleBtn = new Button { Text = "Schedule", Top = 185, Left = 545, Width = 125, Height = 32 };

        var scheduleHelp = new Label
        {
            Top = 158,
            Left = 445,
            Width = 430,
            Height = 22,
            Text = "Choose a folder Loogal should auto-index while running.",
            Font = new Font("Segoe UI", 8),
            ForeColor = Color.Gray
        };

        timeBox = new TextBox { Top = 185, Left = 440, Width = 85, Height = 30, Text = "23:30" };

        var timeHelp = new Label
        {
            Top = 218,
            Left = 445,
            Width = 260,
            Height = 20,
            Text = "24-hour time, HH:mm",
            Font = new Font("Segoe UI", 8),
            ForeColor = Color.Gray
        };

        var startupCheck = new CheckBox
        {
            Text = "Start Loogal with Windows",
            Top = 245,
            Left = 440,
            Width = 260,
            Height = 25
        };

        progressBar = new ProgressBar
        {
            Top = 265,
            Left = 20,
            Width = 360,
            Height = 18,
            Style = ProgressBarStyle.Blocks
        };

        var searchButton = new Button { Text = "Search Image", Width = 180, Height = 44, Top = 300, Left = 20 };

        statusLabel = new Label
        {
            Top = 310,
            Left = 220,
            Width = 900,
            Height = 30,
            Text = "Ready. Index folders first, then search by image."
        };

        previewBox = new PictureBox
        {
            Top = 360,
            Left = 20,
            Width = 240,
            Height = 360,
            BorderStyle = BorderStyle.FixedSingle,
            SizeMode = PictureBoxSizeMode.Zoom,
            BackColor = Color.White
        };

        resultsPanel = new FlowLayoutPanel
        {
            Top = 360,
            Left = 280,
            Width = 860,
            Height = 440,
            AutoScroll = true,
            WrapContents = true,
            FlowDirection = FlowDirection.LeftToRight,
            BackColor = Color.White
        };

        pickBtn.Click += (_, _) => PickDirectory();
        indexBtn.Click += async (_, _) => await IndexSelectedFolders();
        schedulePickBtn.Click += (_, _) => PickScheduleDirectory();
        scheduleBtn.Click += (_, _) => AddSchedule();
        startupCheck.CheckedChanged += (_, _) => SetStartup(startupCheck.Checked);

        searchButton.Click += async (_, _) =>
        {
            using var dialog = new OpenFileDialog();
            dialog.Filter = "Images|*.png;*.jpg;*.jpeg;*.webp";
            if (dialog.ShowDialog() == DialogResult.OK)
                await TrySearchLocalImage(dialog.FileName);
        };

        DragEnter += (_, e) =>
        {
            if (e.Data != null && e.Data.GetDataPresent(DataFormats.FileDrop))
                e.Effect = DragDropEffects.Copy;
        };

        DragDrop += async (_, e) =>
        {
            if (e.Data == null) return;
            var files = e.Data.GetData(DataFormats.FileDrop) as string[];
            if (files is { Length: > 0 })
                await TrySearchLocalImage(files[0]);
        };

        KeyDown += async (_, e) =>
        {
            if (e.Control && e.KeyCode == Keys.V)
                await HandlePaste();
        };

        Controls.Add(intro);
        Controls.Add(pickBtn);
        Controls.Add(pickHelp);
        Controls.Add(indexBtn);
        Controls.Add(selectedDirsLabel);
        Controls.Add(schedulePickBtn);
        Controls.Add(scheduleHelp);
        Controls.Add(timeBox);
        Controls.Add(timeHelp);
        Controls.Add(scheduleBtn);
        Controls.Add(startupCheck);
        Controls.Add(progressBar);
        Controls.Add(searchButton);
        Controls.Add(statusLabel);
        Controls.Add(previewBox);
        Controls.Add(resultsPanel);
    }

    private void PickDirectory()
    {
        using var dlg = new FolderBrowserDialog();
        dlg.Description = "Pick a folder for Loogal to remember";

        if (dlg.ShowDialog() == DialogResult.OK)
        {
            if (!selectedDirs.Contains(dlg.SelectedPath))
                selectedDirs.Add(dlg.SelectedPath);

            selectedDirsLabel.Text = $"{selectedDirs.Count} folder(s) selected.";
            statusLabel.Text = "Folder selected. Click Index Selected when ready.";
        }
    }

    private async Task IndexSelectedFolders()
    {
        if (selectedDirs.Count == 0)
        {
            MessageBox.Show("Pick at least one folder first.");
            return;
        }

        if (!File.Exists(LoogalExe))
        {
            statusLabel.Text = "loogal.exe not found beside this app.";
            return;
        }

        progressBar.Style = ProgressBarStyle.Marquee;
        statusLabel.Text = "Indexing selected folders...";

        string args = "index --hash-mode native " + string.Join(" ", selectedDirs.Select(d => $"\"{d}\""));

        var result = await RunLoogal(args, captureOutput: true);

        progressBar.Style = ProgressBarStyle.Blocks;

        if (result.ExitCode != 0)
        {
            statusLabel.Text = "Index failed.";
            MessageBox.Show(result.Error + "\n" + result.Output);
            return;
        }

        statusLabel.Text = "Areas memorized. Ask Loogal anytime.";
        MessageBox.Show("Areas memorized.\n\nLoogal can now search these folders by image.", "Loogal");
    }

    private async Task<(int ExitCode, string Output, string Error)> RunLoogal(string args, bool captureOutput)
    {
        var process = new Process();
        process.StartInfo.FileName = LoogalExe;
        process.StartInfo.WorkingDirectory = AppDir;
        process.StartInfo.Arguments = args;
        process.StartInfo.UseShellExecute = false;
        process.StartInfo.CreateNoWindow = true;
        process.StartInfo.RedirectStandardOutput = captureOutput;
        process.StartInfo.RedirectStandardError = captureOutput;

        LogDebug($"RUN: {LoogalExe} {args}");
        LogDebug($"CWD: {AppDir}");

        process.Start();

        string output = captureOutput ? await process.StandardOutput.ReadToEndAsync() : "";
        string error = captureOutput ? await process.StandardError.ReadToEndAsync() : "";

        await process.WaitForExitAsync();

        LogDebug($"EXIT: {process.ExitCode}");
        if (!string.IsNullOrWhiteSpace(output)) LogDebug("STDOUT:\n" + output);
        if (!string.IsNullOrWhiteSpace(error)) LogDebug("STDERR:\n" + error);

        return (process.ExitCode, output, error);
    }

    private void PickScheduleDirectory()
    {
        using var dlg = new FolderBrowserDialog();
        dlg.Description = "Pick a folder to auto-index";

        if (dlg.ShowDialog() == DialogResult.OK)
        {
            scheduledDir = dlg.SelectedPath;
            statusLabel.Text = "Scheduled folder selected.";
        }
    }

    private void AddSchedule()
    {
        if (string.IsNullOrWhiteSpace(scheduledDir))
        {
            MessageBox.Show("Pick a folder to schedule first.");
            return;
        }

        string time = timeBox.Text.Trim();

        if (!System.Text.RegularExpressions.Regex.IsMatch(time, @"^\d{2}:\d{2}$"))
        {
            MessageBox.Show("Use 24-hour time: HH:mm");
            return;
        }

        schedules.Add(new ScheduleItem { Folder = scheduledDir, Time = time, LastRunDate = "" });
        SaveSchedules();

        statusLabel.Text = $"Scheduled daily at {time} while Loogal is running.";
        MessageBox.Show($"Scheduled.\n\nLoogal will auto-index this folder daily at {time} while the app is running.", "Loogal");
    }

    private void StartScheduleTimer()
    {
        var timer = new System.Windows.Forms.Timer();
        timer.Interval = 60000;
        timer.Tick += async (_, _) => await CheckSchedules();
        timer.Start();
    }

    private async Task CheckSchedules()
    {
        string now = DateTime.Now.ToString("HH:mm");
        string today = DateTime.Now.ToString("yyyy-MM-dd");

        foreach (var item in schedules)
        {
            if (item.Time == now && item.LastRunDate != today)
            {
                await RunIndexForFolder(item.Folder);
                item.LastRunDate = today;
                SaveSchedules();
                statusLabel.Text = "Auto-index complete.";
            }
        }
    }

    private async Task RunIndexForFolder(string folder)
    {
        if (!File.Exists(LoogalExe) || !Directory.Exists(folder)) return;
        await RunLoogal($"index --hash-mode native \"{folder}\"", captureOutput: false);
    }

    private void SetStartup(bool enabled)
    {
        using var key = Registry.CurrentUser.OpenSubKey(@"SOFTWARE\Microsoft\Windows\CurrentVersion\Run", true);
        if (key == null) return;

        if (enabled)
            key.SetValue("Loogal", Application.ExecutablePath);
        else
            key.DeleteValue("Loogal", false);
    }

    private void LoadSchedules()
    {
        try
        {
            if (!File.Exists(ScheduleFile)) return;
            var loaded = JsonSerializer.Deserialize<List<ScheduleItem>>(File.ReadAllText(ScheduleFile));
            if (loaded != null) schedules.AddRange(loaded);
        }
        catch { }
    }

    private void SaveSchedules()
    {
        File.WriteAllText(ScheduleFile, JsonSerializer.Serialize(schedules, new JsonSerializerOptions { WriteIndented = true }));
    }

    private async Task HandlePaste()
    {
        if (Clipboard.ContainsImage())
        {
            using var img = Clipboard.GetImage();
            if (img != null)
            {
                string tempDir = Path.Combine(Path.GetTempPath(), "loogal-window");
                Directory.CreateDirectory(tempDir);
                string tempPath = Path.Combine(tempDir, "clipboard-query.png");
                img.Save(tempPath, System.Drawing.Imaging.ImageFormat.Png);
                await TrySearchLocalImage(tempPath);
                return;
            }
        }

        if (Clipboard.ContainsFileDropList())
        {
            var files = Clipboard.GetFileDropList();
            if (files.Count > 0)
            {
                await TrySearchLocalImage(files[0] ?? "");
                return;
            }
        }

        if (Clipboard.ContainsText())
        {
            string text = Clipboard.GetText().Trim();
            if (Uri.TryCreate(text, UriKind.Absolute, out var uri) && (uri.Scheme == "http" || uri.Scheme == "https"))
            {
                try
                {
                    string tempPath = await DownloadUrlToTemp(uri);
                    await TrySearchLocalImage(tempPath);
                    return;
                }
                catch (Exception ex)
                {
                    MessageBox.Show("Could not use clipboard URL as an image.\n\nTry right-click image > Copy image address.\n\n" + ex.Message);
                    return;
                }
            }
        }

        MessageBox.Show("Clipboard has no usable image, image file, or image URL.");
    }

    private async Task<string> DownloadUrlToTemp(Uri uri)
    {
        string tempDir = Path.Combine(Path.GetTempPath(), "loogal-window");
        Directory.CreateDirectory(tempDir);

        using var http = new HttpClient();
        http.DefaultRequestHeaders.UserAgent.ParseAdd("LoogalWindow/1.0");

        using var response = await http.GetAsync(uri);
        response.EnsureSuccessStatusCode();

        string contentType = response.Content.Headers.ContentType?.MediaType ?? "";
        if (!contentType.StartsWith("image/", StringComparison.OrdinalIgnoreCase))
            throw new Exception("URL did not return an image. It returned: " + contentType);

        string ext = contentType.ToLowerInvariant() switch
        {
            "image/jpeg" => ".jpg",
            "image/png" => ".png",
            "image/webp" => ".webp",
            _ => ".png"
        };

        string tempPath = Path.Combine(tempDir, "clipboard-url-query" + ext);
        byte[] bytes = await response.Content.ReadAsByteArrayAsync();
        await File.WriteAllBytesAsync(tempPath, bytes);

        return tempPath;
    }

    private async Task TrySearchLocalImage(string path)
    {
        if (!File.Exists(path))
        {
            statusLabel.Text = "Invalid image path.";
            return;
        }

        if (!IsImageFile(path))
        {
            statusLabel.Text = "Not a supported image file.";
            return;
        }

        await SearchImage(path);
    }

    private bool IsImageFile(string path)
    {
        string ext = Path.GetExtension(path).ToLowerInvariant();
        return ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".webp";
    }

    private async Task SearchImage(string imagePath)
    {
        previewBox.Image?.Dispose();
        previewBox.Image = null;

        try
        {
            using var fs = new FileStream(imagePath, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
            previewBox.Image = Image.FromStream(fs);
        }
        catch
        {
            statusLabel.Text = "Selected file is not a valid image.";
            return;
        }

        resultsPanel.Controls.Clear();
        statusLabel.Text = "Searching Loogal memory...";

        if (!File.Exists(LoogalExe))
        {
            statusLabel.Text = "loogal.exe not found beside this app.";
            return;
        }

        var result = await RunLoogal($"search \"{imagePath}\" 0 --json --limit 50", captureOutput: true);

        if (result.ExitCode != 0)
        {
            statusLabel.Text = "Loogal query failed.";
            MessageBox.Show(result.Error + "\n" + result.Output);
            return;
        }

        JsonDocument json;
        try
        {
            json = JsonDocument.Parse(result.Output);
        }
        catch (Exception ex)
        {
            statusLabel.Text = "Loogal returned invalid JSON.";
            MessageBox.Show("Could not parse Loogal output.\n\n" + ex.Message + "\n\n" + result.Output);
            return;
        }

        using (json)
        {
            double ms = SafeDouble(json.RootElement, "duration_ms");
            long ns = (long)(ms * 1_000_000);
            int candidates = SafeInt(json.RootElement, "candidates");
            int returned = SafeInt(json.RootElement, "returned");

            JsonElement results;
            if (!json.RootElement.TryGetProperty("results", out results) || results.ValueKind != JsonValueKind.Array)
            {
                statusLabel.Text = "No result array returned.";
                return;
            }

            int count = Math.Min(results.GetArrayLength(), 50);
            if (returned == 0) returned = count;

            statusLabel.Text = $"Returned {returned} / {candidates} | {ns:N0} ns | {ms:N3} ms";

            for (int i = 0; i < count; i++)
            {
                var item = results[i];
                string path = SafeString(item, "path");
                string label = SafeString(item, "label");
                double similarity = SafeDouble(item, "similarity_percent");
                int rank = SafeInt(item, "rank");
                if (rank == 0) rank = i + 1;

                if (!string.IsNullOrWhiteSpace(path))
                    resultsPanel.Controls.Add(BuildResultCard(rank, path, label, similarity));
            }

            if (count == 0)
                statusLabel.Text = "No matches found.";
        }
    }

    private Panel BuildResultCard(int rank, string path, string label, double similarity)
    {
        var card = new Panel
        {
            Width = 230,
            Height = 310,
            Margin = new Padding(10),
            BackColor = Color.FromArgb(250, 250, 250),
            BorderStyle = BorderStyle.FixedSingle
        };

        var pic = new PictureBox
        {
            Top = 10,
            Left = 10,
            Width = 208,
            Height = 200,
            SizeMode = PictureBoxSizeMode.Zoom,
            BackColor = Color.White
        };

        try
        {
            using var fs = new FileStream(path, FileMode.Open, FileAccess.Read, FileShare.ReadWrite);
            pic.Image = Image.FromStream(fs);
        }
        catch
        {
            pic.BackColor = Color.LightGray;
        }

        var title = new Label
        {
            Top = 220,
            Left = 10,
            Width = 208,
            Height = 20,
            Text = $"#{rank}  {similarity:N2}%  {label}",
            Font = new Font("Segoe UI", 9, FontStyle.Bold)
        };

        var file = new Label
        {
            Top = 245,
            Left = 10,
            Width = 208,
            Height = 35,
            Text = Path.GetFileName(path),
            Font = new Font("Segoe UI", 8)
        };

        var openButton = new Button { Text = "Open", Top = 280, Left = 10, Width = 95, Height = 24 };
        openButton.Click += (_, _) =>
        {
            if (File.Exists(path))
                Process.Start(new ProcessStartInfo(path) { UseShellExecute = true });
        };

        var revealButton = new Button { Text = "Reveal", Top = 280, Left = 123, Width = 95, Height = 24 };
        revealButton.Click += (_, _) =>
        {
            if (File.Exists(path))
                Process.Start("explorer.exe", $"/select,\"{path}\"");
        };

        card.Controls.Add(pic);
        card.Controls.Add(title);
        card.Controls.Add(file);
        card.Controls.Add(openButton);
        card.Controls.Add(revealButton);

        return card;
    }
}
