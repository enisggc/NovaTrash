using Microsoft.EntityFrameworkCore;
using NovaTrash.Models;

namespace NovaTrash.Data
{
    public class NovaTrashDbContext : DbContext
    {
        public NovaTrashDbContext(DbContextOptions<NovaTrashDbContext> options) : base(options) { }

        public DbSet<SensorReading> SensorReadings { get; set; }

        protected override void OnModelCreating(ModelBuilder modelBuilder)
        {
            modelBuilder.Entity<SensorReading>(entity =>
            {
                entity.HasKey(e => e.Id);
                entity.Property(e => e.BinId).IsRequired().HasMaxLength(20);
                entity.Property(e => e.BinName).IsRequired().HasMaxLength(20);
                entity.HasIndex(e => e.Timestamp);
                entity.HasIndex(e => e.BinId);
            });
        }
    }
}