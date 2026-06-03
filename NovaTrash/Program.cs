var builder = WebApplication.CreateBuilder(args);

// Controller desteği
builder.Services.AddControllers();
builder.Services.AddEndpointsApiExplorer();

var app = builder.Build();

// API'nin (ESP32) gelen isteklere açılması
app.Urls.Add("http://0.0.0.0:5120");

if (app.Environment.IsDevelopment())
{
    app.UseDeveloperExceptionPage();
}

app.UseAuthorization();
app.MapControllers();

Console.WriteLine("--- NOVATRASH API BASLATILDI ---");
Console.WriteLine("Dinlenen Adres: http://0.0.0.0:5120");

app.Run();