namespace NovaTrash.Models
{
    public class SensorReading
    {
        public int Id { get; set; }
        public string BinId { get; set; } = "";      
        public string BinName { get; set; } = "";     
        public int Status { get; set; }       
        public long WeightG { get; set; }
        public DateTime Timestamp { get; set; } = DateTime.Now;
    }
}
 