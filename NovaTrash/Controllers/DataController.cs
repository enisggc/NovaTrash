using Microsoft.AspNetCore.Mvc;
using Microsoft.Extensions.Logging;
using System.Linq;

namespace NovaTrash.Controllers
{
    [Route("api/[controller]")]
    [ApiController]
    public class DataController : ControllerBase
    {
        [HttpPost("esp32-upload")]
        public async Task<IActionResult> Esp32Upload()
        {
            try
            {
                // ESP32'den geleni oku
                using var ms = new MemoryStream();
                await Request.Body.CopyToAsync(ms);
                var bytes = ms.ToArray();

                if (bytes.Length == 0)
                {
                    Console.WriteLine("[HATA] Bos veri geldi!");
                    return BadRequest("Görüntü boş.");
                }

                // ML.NET Tahmini
                var input = new WasteModel.ModelInput() { ImageSource = bytes };
                var prediction = WasteModel.Predict(input);

                // Konsola yazdır
                string sonuc = $"[ANALIZ] Tur: {prediction.PredictedLabel} | Oran: %{prediction.Score.Max() * 100:F2}";
                Console.WriteLine(sonuc);

                // ESP32'ye yanıt dön
                return Ok($"{prediction.PredictedLabel}");
            }
            catch (Exception ex)
            {
                Console.WriteLine($"[HATA] {ex.Message}");
                return StatusCode(500, ex.Message);
            }
        }
    }
}