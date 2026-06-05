using Microsoft.AspNetCore.Mvc;
using Microsoft.EntityFrameworkCore;
using NovaTrash.Data;
using NovaTrash.Models;

namespace NovaTrash.Controllers
{
    // Kutu adları
    public static class BinNames
    {
        public static readonly Dictionary<string, string> Map = new()
        {
            { "kagit",   "Kağıt"   },
            { "plastik", "Plastik" },
            { "cam",     "Cam"     },
            { "metal",   "Metal"   },
        };
    }

    // ESP32'den gelen POST body
    public class SensorPayload
    {
        public string BinId { get; set; } = ""; //hangi kutu
        public int Status { get; set; } //doluluk
        public long WeightG { get; set; } //ağırlık
    }

    [Route("api/[controller]")]
    [ApiController]
    public class SensorController : ControllerBase
    {
        private readonly NovaTrashDbContext _db;

        public SensorController(NovaTrashDbContext db)
        {
            _db = db;
        }

        // ESP32 --> POST /api/sensor/data
        // data endpoint 
        [HttpPost("data")]
        public async Task<IActionResult> PostSensorData([FromBody] SensorPayload payload)
        {
            if (payload == null || string.IsNullOrWhiteSpace(payload.BinId))
                return BadRequest("BinId boş olamaz.");

            if (!BinNames.Map.ContainsKey(payload.BinId))
                return BadRequest($"Geçersiz BinId: '{payload.BinId}'. Geçerli: kagit, plastik, cam, metal");

            var reading = new SensorReading
            {
                BinId     = payload.BinId,
                BinName   = BinNames.Map[payload.BinId],
                Status    = payload.Status,
                WeightG   = payload.WeightG,
                Timestamp = DateTime.Now
            };

            _db.SensorReadings.Add(reading);
            await _db.SaveChangesAsync();

            //consolea yazdır.
            Console.WriteLine($"[SENSÖR] {reading.BinName} | STATUS:{reading.Status} | WEIGHT:{reading.WeightG}g | {reading.Timestamp:HH:mm:ss}");

            return Ok(new { message = "Kaydedildi.", id = reading.Id, timestamp = reading.Timestamp });
        }

        // Dashboard --> GET /api/sensor/latest
        // Her kutunun en son kaydını 3 dkda bir döner.
        [HttpGet("latest")]
        public async Task<IActionResult> GetLatest()
        {
            //kutu tiplerine göre sıralama tarihe göre azalan.
            var latest = await _db.SensorReadings
                .GroupBy(r => r.BinId)
                .Select(g => g.OrderByDescending(r => r.Timestamp).First()) //azalan bir sıralama
                .ToListAsync();

            // Henüz kaydı olmayan kutular için varsayılan döndür
            var result = BinNames.Map.Select(kv =>
            {
                var found = latest.FirstOrDefault(r => r.BinId == kv.Key);
                return found ?? new SensorReading
                {
                    BinId     = kv.Key,
                    BinName   = kv.Value,
                    Status    = 0,
                    WeightG   = 0,
                    Timestamp = DateTime.MinValue
                };
            }).ToList();

            return Ok(result);
        }

        // Geçmiş kayıtları yeniden eskiye sırala.
        [HttpGet("history")]
        public async Task<IActionResult> GetHistory(
            [FromQuery] string? binId,
            [FromQuery] string? durum,
            [FromQuery] int limit = 100)
        {
            var query = _db.SensorReadings.AsQueryable();

            if (!string.IsNullOrWhiteSpace(binId))
                query = query.Where(r => r.BinId == binId);

            if (durum == "dolu")
                query = query.Where(r => r.Status == 1);
            else if (durum == "bos")
                query = query.Where(r => r.Status == 0);

            var result = await query
                .OrderByDescending(r => r.Timestamp)
                .Take(limit)
                .ToListAsync();

            return Ok(result);
        }
    }
}