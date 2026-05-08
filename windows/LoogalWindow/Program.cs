using System.Diagnostics;
using System.Text.Json;
using System.Text.RegularExpressions;


public static class Program
{
    [STAThread]
    public static void Main()
    {
        Directory.SetCurrentDirectory(AppContext.BaseDirectory);
        ApplicationConfiguration.Initialize();
        Application.Run(new LoogalWindow());
    }
}

public class LoogalWindow : Form
{
    readonly FlowLayoutPanel grid = new() { Dock = DockStyle.Fill, AutoScroll = true };
    readonly Label status = new() { Dock = DockStyle.Top, Height = 44, Text = "Drop an image here or click Open Image.", TextAlign = ContentAlignment.MiddleCenter };
    readonly Button openButton = new() { Text = "Open Image...", Dock = DockStyle.Top, Height = 40 };
    readonly Button indexButton = new() { Text = "Add Folders (Index)", Dock = DockStyle.Top, Height = 40 };
    readonly string exe = Path.Combine(AppContext.BaseDirectory, "loogal.exe");
    string currentQuery = "";
    int currentOffset = 0;
    const int PageSize = 20;
    readonly Button loadMoreButton = new() { Text = "Load More", Width = 180, Height = 40, Margin = new Padding(12) };

    public LoogalWindow()
    {
        Text = "Loogal — Click. Drop. Done.";
        Width = 1100;
        Height = 760;
        AllowDrop = true;

        Controls.Add(grid);
        Controls.Add(openButton);
        Controls.Add(indexButton);
        Controls.Add(status);

        openButton.Click += (_, __) =>
        {
            using var dlg = new OpenFileDialog();
            dlg.Filter = "Images|*.png;*.jpg;*.jpeg;*.webp;*.bmp;*.gif";

            if (dlg.ShowDialog() == DialogResult.OK)
            {
                Search(dlg.FileName);
            }
        };

        loadMoreButton.Click += (_, __) => LoadMoreResults();

        indexButton.Click += (_, __) =>
        {
            using var dlg = new FolderBrowserDialog();
            dlg.Description = "Choose a folder for Loogal to remember.";

            if (dlg.ShowDialog() == DialogResult.OK)
            {
                IndexFolder(dlg.SelectedPath);
            }
        };

        DragEnter += (_, e) =>
        {
            if (e.Data?.GetDataPresent(DataFormats.FileDrop) == true)
                e.Effect = DragDropEffects.Copy;
        };

        DragDrop += (_, e) =>
        {
            try
            {
                var files = e.Data?.GetData(DataFormats.FileDrop) as string[];
                if (files is { Length: > 0 }) Search(files[0]);
            }
            catch (Exception ex)
            {
                status.Text = "Drop failed: " + ex.Message;
                MessageBox.Show(ex.ToString(), "Loogal drop error");
            }
        };
    }

    void IndexFolder(string folder)
    {
        try
        {
            if (!File.Exists(exe))
            {
                status.Text = "Missing loogal.exe beside LoogalWindow.exe";
                return;
            }

            status.Text = "Indexing folder... Loogal is remembering.";

            indexButton.Enabled = false;
            openButton.Enabled = false;

            Task.Run(() =>
            {
                string output = Run("index", folder);

                BeginInvoke(() =>
                {
                    indexButton.Enabled = true;
                    openButton.Enabled = true;
                    status.Text = "Index complete. Open an image to search.";
                    MessageBox.Show(output.Length > 2000 ? output[..2000] : output, "Loogal index complete");
                });
            });
        }
        catch (Exception ex)
        {
            indexButton.Enabled = true;
            openButton.Enabled = true;
            status.Text = "Index failed: " + ex.Message;
            MessageBox.Show(ex.ToString(), "Loogal index error");
        }
    }

    void Search(string query)
    {
        try
        {
            currentQuery = query;
            currentOffset = 0;
            grid.Controls.Clear();
            LoadMoreResults();
        }
        catch (Exception ex)
        {
            status.Text = "Search failed: " + ex.Message;
            MessageBox.Show(ex.ToString(), "Loogal search error");
        }
    }

    void LoadMoreResults()
    {
        try
        {
            if (!File.Exists(exe))
            {
                status.Text = "Missing loogal.exe beside LoogalWindow.exe";
                return;
            }

            if (string.IsNullOrWhiteSpace(currentQuery))
            {
                status.Text = "Open an image first.";
                return;
            }

            if (grid.Controls.Contains(loadMoreButton))
                grid.Controls.Remove(loadMoreButton);

            status.Text = $"Loading results {currentOffset + 1} - {currentOffset + PageSize}...";

            string searchJson = Run(
                "search",
                currentQuery,
                "--memory",
                "--json",
                "--offset",
                currentOffset.ToString(),
                "--limit",
                PageSize.ToString()
            );

            var paths = Regex.Matches(searchJson, "\"path\"\\s*:\\s*\"(.*?)\"")
                .Select(m => Regex.Unescape(m.Groups[1].Value))
                .Distinct()
                .ToList();

            if (paths.Count == 0)
            {
                status.Text = currentOffset == 0 ? "0 results" : "No more results.";
                return;
            }

            foreach (var path in paths)
                AddResultCard(path);

            currentOffset += paths.Count;
            status.Text = $"{currentOffset} results loaded";

            if (paths.Count == PageSize)
                grid.Controls.Add(loadMoreButton);
        }
        catch (Exception ex)
        {
            status.Text = "Load more failed: " + ex.Message;
            MessageBox.Show(ex.ToString(), "Loogal load more error");
        }
    }

    void AddResultCard(string path)
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

    string Run(params string[] args)
    {
        Directory.SetCurrentDirectory(AppContext.BaseDirectory);
        Directory.CreateDirectory(Path.Combine(AppContext.BaseDirectory, "data"));
        Directory.CreateDirectory(Path.Combine(AppContext.BaseDirectory, "data", "logs"));
        Directory.CreateDirectory(Path.Combine(AppContext.BaseDirectory, "manifests"));

        var psi = new ProcessStartInfo(exe)
        {
            WorkingDirectory = AppContext.BaseDirectory,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
            CreateNoWindow = true
        };

        foreach (var a in args) psi.ArgumentList.Add(a);

        string debugPath = Path.Combine(AppContext.BaseDirectory, "loogal-gui-debug.txt");

        using var p = Process.Start(psi)!;
        string stdout = p.StandardOutput.ReadToEnd();
        string stderr = p.StandardError.ReadToEnd();
        p.WaitForExit();

        File.AppendAllText(debugPath,
            "\n--- LOOGAL GUI RUN ---\n" +
            "BaseDirectory: " + AppContext.BaseDirectory + "\n" +
            "CurrentDirectory: " + Directory.GetCurrentDirectory() + "\n" +
            "Exe: " + exe + "\n" +
            "Args: " + string.Join(" ", args.Select(a => "\"" + a + "\"")) + "\n" +
            "ExitCode: " + p.ExitCode + "\n" +
            "STDOUT:\n" + stdout + "\n" +
            "STDERR:\n" + stderr + "\n"
        );

        if (p.ExitCode != 0)
            return "LOOGAL GUI ERROR\nExitCode: " + p.ExitCode + "\n\nSTDOUT:\n" + stdout + "\n\nSTDERR:\n" + stderr;

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
