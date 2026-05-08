using Avalonia.Controls;
using Avalonia.Media.Imaging;
using Avalonia.Input;
using Avalonia.Media;
using Avalonia;
using System.IO;

namespace locus_desktop;

public partial class ResultsWindow : Window
{
    public ResultsWindow(LoogalSearchResponse response)
    {
        InitializeComponent();

        HeaderText.Text = $"LOCUS Results — showing {response.Results.Count} of {response.TotalHits} hits";

        foreach (var r in response.Results)
            ResultsWrap.Children.Add(BuildCard(r));

        Opened += (_, _) => Focus();
        KeyDown += OnKeyDown;
    }

    private Control BuildCard(LoogalResult r)
    {
        var image = new Image
        {
            Width = 240,
            Height = 180,
            Stretch = Avalonia.Media.Stretch.UniformToFill
        };

        try
        {
            if (File.Exists(r.Path))
                image.Source = new Bitmap(r.Path);
        }
        catch {}

        var title = new TextBlock
        {
            Text = $"{r.SimilarityPercent:0.##}% {r.Label}",
            FontWeight = Avalonia.Media.FontWeight.Bold
        };

        var name = new TextBlock
        {
            Text = Path.GetFileName(r.Path),
            TextWrapping = Avalonia.Media.TextWrapping.Wrap,
            MaxWidth = 220
        };

        var dir = new TextBlock
        {
            Text = Path.GetDirectoryName(r.Path) ?? "",
            TextWrapping = Avalonia.Media.TextWrapping.Wrap,
            MaxWidth = 220,
            FontSize = 11,
            Opacity = 0.65
        };

        var stack = new StackPanel
        {
            Width = 250,
            Margin = new Avalonia.Thickness(10),
            Children = { image, title, name, dir }
        };

        var border = new Border
        {
            CornerRadius = new Avalonia.CornerRadius(14),
            Padding = new Avalonia.Thickness(10),
            Child = stack,
            Focusable = true,
            RenderTransformOrigin = new RelativePoint(0.5, 0.5, RelativeUnit.Relative),
            RenderTransform = new ScaleTransform(1.0, 1.0)
        };

        border.PointerEntered += (_, _) =>
        {
            border.ZIndex = 100;
            border.RenderTransform = new ScaleTransform(1.10, 1.10);
            border.Background = new SolidColorBrush(Color.FromArgb(28, 255, 255, 255));
        };

        border.PointerExited += (_, _) =>
        {
            border.ZIndex = 0;
            border.RenderTransform = new ScaleTransform(1.0, 1.0);
            border.Background = Brushes.Transparent;
        };

        border.DoubleTapped += (_, _) => LoogalCli.Reveal(r.Path);

        border.PointerPressed += (_, e) =>
        {
            border.Focus();
            border.RenderTransform = new ScaleTransform(1.14, 1.14);

            if (e.ClickCount >= 2)
                LoogalCli.Reveal(r.Path);
        };

        return border;
    }

    private void OnKeyDown(object? sender, KeyEventArgs e)
    {
        var step = 90.0;

        if (e.Key == Key.Down)
        {
            ResultsScroll.Offset = ResultsScroll.Offset.WithY(ResultsScroll.Offset.Y + step);
            e.Handled = true;
        }
        else if (e.Key == Key.Up)
        {
            ResultsScroll.Offset = ResultsScroll.Offset.WithY(ResultsScroll.Offset.Y - step);
            e.Handled = true;
        }
        else if (e.Key == Key.PageDown)
        {
            ResultsScroll.Offset = ResultsScroll.Offset.WithY(ResultsScroll.Offset.Y + 600);
            e.Handled = true;
        }
        else if (e.Key == Key.PageUp)
        {
            ResultsScroll.Offset = ResultsScroll.Offset.WithY(ResultsScroll.Offset.Y - 600);
            e.Handled = true;
        }
    }
}
