using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Platform.Storage;
using System;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Text.RegularExpressions;
using System.Threading.Tasks;

namespace locus_desktop;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();

        ChooseButton.Click += ChooseImage;
        IndexButton.Click += IndexDirectory;
    }

    private static bool IsSupportedImage(string path)
    {
        var ext = Path.GetExtension(path).ToLowerInvariant();

        return ext is ".png" or ".jpg" or ".jpeg" or ".webp";
    }

    private static int CountImages(string root)
    {
        try
        {
            return Directory.EnumerateFiles(root, "*", SearchOption.AllDirectories)
                .Count(IsSupportedImage);
        }
        catch
        {
            return 0;
        }
    }

    private static string RepoRoot()
    {
        return Path.GetFullPath(Path.Combine(
            Directory.GetCurrentDirectory(),
            "..",
            ".."
        ));
    }

    private static int ReadLatestScannedCount(string repoRoot)
    {
        var logsDir = Path.Combine(repoRoot, "logs");

        if (!Directory.Exists(logsDir))
            return 0;

        try
        {
            var files = Directory.GetFiles(logsDir, "*", SearchOption.AllDirectories)
                .OrderByDescending(File.GetLastWriteTimeUtc)
                .Take(4);

            var best = 0;
            var rx = new Regex(@"scanned=(\d+)", RegexOptions.Compiled);

            foreach (var file in files)
            {
                foreach (var line in File.ReadLines(file).Reverse().Take(300))
                {
                    var m = rx.Match(line);
                    if (m.Success && int.TryParse(m.Groups[1].Value, out var n))
                        best = Math.Max(best, n);
                }
            }

            return best;
        }
        catch
        {
            return 0;
        }
    }

    private async void IndexDirectory(object? sender, RoutedEventArgs e)
    {
        var folders = await StorageProvider.OpenFolderPickerAsync(
            new FolderPickerOpenOptions
            {
                Title = "Select directory to index",
                AllowMultiple = false
            });

        var folder = folders.FirstOrDefault();

        if (folder?.Path.LocalPath is not { Length: > 0 } path)
            return;

        try
        {
            IndexButton.IsEnabled = false;
            ChooseButton.IsEnabled = false;

            IndexProgress.IsVisible = true;
            ProgressText.IsVisible = true;
            IndexProgress.Value = 0;

            StatusText.Text = $"Scanning directory first:\n{path}";
            ProgressText.Text = "Counting images...";

            var totalImages = await Task.Run(() => CountImages(path));

            if (totalImages <= 0)
            {
                StatusText.Text = "No supported images found in that directory.";
                ProgressText.Text = "";
                IndexProgress.Value = 0;
                return;
            }

            var repoRoot = RepoRoot();
            var engine = LoogalCli.FindEngine();

            StatusText.Text =
                $"Indexing directory:\n{path}\n\nDetected {totalImages} supported images.";
            ProgressText.Text = $"0 / {totalImages} images indexed · 0%";

            var psi = new ProcessStartInfo
            {
                FileName = engine,
                WorkingDirectory = repoRoot,
                UseShellExecute = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true
            };

            psi.ArgumentList.Add("index");
            psi.ArgumentList.Add(path);

            using var proc = Process.Start(psi);

            if (proc == null)
            {
                StatusText.Text = "Failed to start loogal engine.";
                return;
            }

            var progressTask = Task.Run(async () =>
            {
                var fake = 0.0;

                while (!proc.HasExited)
                {
                    fake += 1.5;

                    if (fake > 96.0)
                        fake = 96.0;

                    var pct = fake;

                    await Avalonia.Threading.Dispatcher.UIThread.InvokeAsync(() =>
                    {
                        IndexProgress.Value = pct;
                        ProgressText.Text =
                            $"Indexing {totalImages} detected images · {pct:0.0}%";
                    });

                    await Task.Delay(180);
                }
            });

            var stdoutTask = proc.StandardOutput.ReadToEndAsync();
            var stderrTask = proc.StandardError.ReadToEndAsync();

            await proc.WaitForExitAsync();
            await progressTask;

            var stdout = await stdoutTask;
            var stderr = await stderrTask;

            if (proc.ExitCode == 0)
            {
                IndexProgress.Value = 100;
                ProgressText.Text = $"{totalImages} / {totalImages} images indexed · 100%";

                StatusText.Text =
                    "INDEX COMPLETE\n\n" +
                    (string.IsNullOrWhiteSpace(stdout) ? "(no output)" : stdout.Trim());
            }
            else
            {
                StatusText.Text =
                    "INDEX FAILED\n\n" +
                    (string.IsNullOrWhiteSpace(stderr) ? stdout.Trim() : stderr.Trim());
            }
        }
        catch (Exception ex)
        {
            StatusText.Text = "INDEX ERROR\n\n" + ex.Message;
        }
        finally
        {
            IndexButton.IsEnabled = true;
            ChooseButton.IsEnabled = true;
        }
    }

    private async void ChooseImage(object? sender, RoutedEventArgs e)
    {
        var files = await StorageProvider.OpenFilePickerAsync(
            new FilePickerOpenOptions
            {
                Title = "Choose image",
                AllowMultiple = false
            });

        var file = files.FirstOrDefault();

        if (file?.Path.LocalPath is { Length: > 0 } path)
            await Search(path);
    }

    private async Task Search(string path)
    {
        try
        {
            StatusText.Text = $"Searching {Path.GetFileName(path)} ...";

            var response = await LoogalCli.SearchAsync(path);

            StatusText.Text =
                $"Found {response.TotalHits} results";

            var win = new ResultsWindow(response);

            win.Show();
        }
        catch (Exception ex)
        {
            StatusText.Text = ex.Message;
        }
    }
}
