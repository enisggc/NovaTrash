using Microsoft.EntityFrameworkCore;
using NovaTrash.Data;

var builder = WebApplication.CreateBuilder(args);

builder.Services.AddControllers();
builder.Services.AddEndpointsApiExplorer();

// SQLite + EF Core bağlantısı 
builder.Services.AddDbContext<NovaTrashDbContext>(options =>
    options.UseSqlite("Data Source=novatrash.db"));

// Dashboard farklı porttan istek atacağı çin CORSi- api ile main bağlantısı 
builder.Services.AddCors(options =>
{
    options.AddPolicy("DashboardPolicy", policy =>
    {
        policy.AllowAnyOrigin()
              .AllowAnyMethod()
              .AllowAnyHeader();
    });
});

var app = builder.Build();

// Uygulama ile data aynı anda başlar
using (var scope = app.Services.CreateScope())
{
    var db = scope.ServiceProvider.GetRequiredService<NovaTrashDbContext>();
    db.Database.EnsureCreated();
}

app.Urls.Add("http://0.0.0.0:5120");

if (app.Environment.IsDevelopment())
    app.UseDeveloperExceptionPage();

app.UseCors("DashboardPolicy");
app.UseDefaultFiles();
app.UseStaticFiles(); 
app.UseAuthorization();
app.MapControllers();

Console.WriteLine("--- NOVATRASH API BASLATILDI ---");
Console.WriteLine("Adres          : http://0.0.0.0:5120");
Console.WriteLine("Veritabanı     : novatrash.db (SQLite)");
Console.WriteLine("Sensör POST    : /api/sensor/data");
Console.WriteLine("Son durum GET  : /api/sensor/latest");
Console.WriteLine("Geçmiş GET     : /api/sensor/history");

app.Run();