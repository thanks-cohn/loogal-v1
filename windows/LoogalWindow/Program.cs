using System.Diagnostics;
using System.Text.Json;
using System.Text.RegularExpressions;

ApplicationConfiguration.Initialize();
Application.Run(new LoogalWindow());

public class LoogalWindow : Form
{
    readonly FlowLayoutPanel grid = new() { Dock = DockStyle.Fill, AutoScroll = true };
    readonly Label status = new() { Dock = DockStyle.Top, Height = 44, Text = "Drop an image here. Loogal is hungry.", TextAlign = ContentAlignment.MiddleCenter };
    readonly string exe = Path.Combine(AppContext.BaseDirectory, "loogal.exe");

    public LoogalWindow()
    {
        Text = "Loogal — Click. Drop. Done.";
        Width = 1100;
        Height = 760;
        AllowDrop = true;

        Controls.Add(grid);
        Controls.Add(status);

        DragEnter += (_, e) =>
        {
            if (e.Data?.GetDataPresent(DataFormats.FileDrop) == true)
                e.Effect = DragDropEffects.Copy;
        };

        DragDrop += (_, e) =>
        {
            var files = e.Data?.GetData(DataFormats.FileDrop) as string[];
            if (files is { Length: > 0 }) Search(files[0]);
        };
    }

    void Search(string query)
    {
        grid.Controls.Clear();

        if (!File.Exists(exe))
        {
            status.Text = "Missing loogal.exe beside LoogalWindow.exe";
            return;
        }

        string place = Environment.GetFolderPath(Environment.SpecialFolder.MyPictures);
        status.Text = "Searching memory...";

        string sessionJson = Run("session", "create", "--query", query, "--place", place, "--json");
        string? id = JsonString(sessionJson, "id");

        if (string.IsNullOrWhiteSpace(id))
        {
            status.Text = "No session id. Raw: " + Clip(sessionJson);
            return;
        }

        string pageJson = Run("window-api", "page", "--session", id, "--offset", "0", "--limit", "50", "--json");
        var paths = Regex.Matches(pageJson, "\"path\"\\s*:\\s*\"(.*?)\"")
            .Select(m => Regex.Unescape(m.Groups[1].Value))
            .Distinct()
            .ToList();

        status.Text = $"Session {id} — {paths.Count} results";

        foreach (var path in paths)
        {
            var box = new Panel { Width = 190, Height = 210, BorderStyle = BorderStyle.FixedSingle, Margin = new Padding(8), Cursor = Cursors.Hand };
            var img = new PictureBox { Dock = DockStyle.Top, Height = 150, SizeMode = PictureBoxSizeMode.Zoom };
            var label = new Label { Dock = DockStyle.Fill, Text = Path.GetFileName(path), TextAlign = ContentAlignment.MiddleCenter };

            try { if (File.Exists(path)) img.Image = Image.FromFile(path); } catch { }

            box.Controls.Add(label);
            box.Controls.Add(img);

            box.DoubleClick += (_, _) =>
            {
                try { Process.Start(new ProcessStartInfo("explorer.exe", $"/select,\"{path}\"") { UseShellExecute = true }); }
                catch { }
            };

            grid.Controls.Add(box);
        }
    }

    string Run(params string[] args)
    {
        var psi = new ProcessStartInfo(exe)
        {
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true
        };

        foreach (var a in args) psi.ArgumentList.Add(a);

        using var p = Process.Start(psi)!;
        string stdout = p.StandardOutput.ReadToEnd();
        string stderr = p.StandardError.ReadToEnd();
        p.WaitForExit();

        return stdout.Length > 0 ? stdout : stderr;
    }

    static string? JsonString(string json, string key)
    {
        try
        {
            using var doc = JsonDocument.Parse(json);
            if (doc.RootElement.TryGetProperty(key, out var v)) return v.GetString();
        }
        catch { }

        var m = Regex.Match(json, $"\"{key}\"\\s*:\\s*\"(.*?)\"");
        return m.Success ? Regex.Unescape(m.Groups[1].Value) : null;
    }

    static string Clip(string s) => s.Length > 240 ? s[..240] : s;
}
