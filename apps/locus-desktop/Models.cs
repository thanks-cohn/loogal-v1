using System.Collections.Generic;
using System.Text.Json.Serialization;

public sealed class LoogalSearchResponse
{
    [JsonPropertyName("total_hits")] public int TotalHits { get; set; }
    [JsonPropertyName("returned")] public int Returned { get; set; }
    [JsonPropertyName("duration_seconds")] public double DurationSeconds { get; set; }
    [JsonPropertyName("images_per_second")] public double ImagesPerSecond { get; set; }
    [JsonPropertyName("results")] public List<LoogalResult> Results { get; set; } = new();
}

public sealed class LoogalResult
{
    [JsonPropertyName("rank")] public int Rank { get; set; }
    [JsonPropertyName("path")] public string Path { get; set; } = "";
    [JsonPropertyName("label")] public string Label { get; set; } = "";
    [JsonPropertyName("similarity_percent")] public double SimilarityPercent { get; set; }
    [JsonPropertyName("width")] public int Width { get; set; }
    [JsonPropertyName("height")] public int Height { get; set; }
}
