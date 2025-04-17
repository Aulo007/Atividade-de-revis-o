#include "ssd1306.h"
#include "font.h"

void ssd1306_init(ssd1306_t *ssd, uint8_t width, uint8_t height, bool external_vcc, uint8_t address, i2c_inst_t *i2c)
{
  ssd->width = width;
  ssd->height = height;
  ssd->pages = height / 8U;
  ssd->address = address;
  ssd->i2c_port = i2c;
  ssd->bufsize = ssd->pages * ssd->width + 1;
  ssd->ram_buffer = calloc(ssd->bufsize, sizeof(uint8_t));
  ssd->ram_buffer[0] = 0x40;
  ssd->port_buffer[0] = 0x80;
}

void ssd1306_config(ssd1306_t *ssd)
{
  ssd1306_command(ssd, SET_DISP | 0x00);
  ssd1306_command(ssd, SET_MEM_ADDR);
  ssd1306_command(ssd, 0x01);
  ssd1306_command(ssd, SET_DISP_START_LINE | 0x00);
  ssd1306_command(ssd, SET_SEG_REMAP | 0x01);
  ssd1306_command(ssd, SET_MUX_RATIO);
  ssd1306_command(ssd, HEIGHT - 1);
  ssd1306_command(ssd, SET_COM_OUT_DIR | 0x08);
  ssd1306_command(ssd, SET_DISP_OFFSET);
  ssd1306_command(ssd, 0x00);
  ssd1306_command(ssd, SET_COM_PIN_CFG);
  ssd1306_command(ssd, 0x12);
  ssd1306_command(ssd, SET_DISP_CLK_DIV);
  ssd1306_command(ssd, 0x80);
  ssd1306_command(ssd, SET_PRECHARGE);
  ssd1306_command(ssd, 0xF1);
  ssd1306_command(ssd, SET_VCOM_DESEL);
  ssd1306_command(ssd, 0x30);
  ssd1306_command(ssd, SET_CONTRAST);
  ssd1306_command(ssd, 0xFF);
  ssd1306_command(ssd, SET_ENTIRE_ON);
  ssd1306_command(ssd, SET_NORM_INV);
  ssd1306_command(ssd, SET_CHARGE_PUMP);
  ssd1306_command(ssd, 0x14);
  ssd1306_command(ssd, SET_DISP | 0x01);
}

void ssd1306_command(ssd1306_t *ssd, uint8_t command)
{
  ssd->port_buffer[1] = command;
  i2c_write_blocking(
      ssd->i2c_port,
      ssd->address,
      ssd->port_buffer,
      2,
      false);
}

void ssd1306_send_data(ssd1306_t *ssd)
{
  ssd1306_command(ssd, SET_COL_ADDR);
  ssd1306_command(ssd, 0);
  ssd1306_command(ssd, ssd->width - 1);
  ssd1306_command(ssd, SET_PAGE_ADDR);
  ssd1306_command(ssd, 0);
  ssd1306_command(ssd, ssd->pages - 1);
  i2c_write_blocking(
      ssd->i2c_port,
      ssd->address,
      ssd->ram_buffer,
      ssd->bufsize,
      false);
}

void ssd1306_pixel(ssd1306_t *ssd, uint8_t x, uint8_t y, bool value)
{
  uint16_t index = (y >> 3) + (x << 3) + 1;
  uint8_t pixel = (y & 0b111);
  if (value)
    ssd->ram_buffer[index] |= (1 << pixel);
  else
    ssd->ram_buffer[index] &= ~(1 << pixel);
}

/*
void ssd1306_fill(ssd1306_t *ssd, bool value) {
  uint8_t byte = value ? 0xFF : 0x00;
  for (uint8_t i = 1; i < ssd->bufsize; ++i)
    ssd->ram_buffer[i] = byte;
}*/

void ssd1306_fill(ssd1306_t *ssd, bool value)
{
  // Itera por todas as posições do display
  for (uint8_t y = 0; y < ssd->height; ++y)
  {
    for (uint8_t x = 0; x < ssd->width; ++x)
    {
      ssd1306_pixel(ssd, x, y, value);
    }
  }
}

void ssd1306_rect(ssd1306_t *ssd, uint8_t top, uint8_t left, uint8_t width, uint8_t height, bool value, bool fill)
{
  for (uint8_t x = left; x < left + width; ++x)
  {
    ssd1306_pixel(ssd, x, top, value);
    ssd1306_pixel(ssd, x, top + height - 1, value);
  }
  for (uint8_t y = top; y < top + height; ++y)
  {
    ssd1306_pixel(ssd, left, y, value);
    ssd1306_pixel(ssd, left + width - 1, y, value);
  }

  if (fill)
  {
    for (uint8_t x = left + 1; x < left + width - 1; ++x)
    {
      for (uint8_t y = top + 1; y < top + height - 1; ++y)
      {
        ssd1306_pixel(ssd, x, y, value);
      }
    }
  }
}

void ssd1306_line(ssd1306_t *ssd, uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1, bool value)
{
  int dx = abs(x1 - x0);
  int dy = abs(y1 - y0);

  int sx = (x0 < x1) ? 1 : -1;
  int sy = (y0 < y1) ? 1 : -1;

  int err = dx - dy;

  while (true)
  {
    ssd1306_pixel(ssd, x0, y0, value); // Desenha o pixel atual

    if (x0 == x1 && y0 == y1)
      break; // Termina quando alcança o ponto final

    int e2 = err * 2;

    if (e2 > -dy)
    {
      err -= dy;
      x0 += sx;
    }

    if (e2 < dx)
    {
      err += dx;
      y0 += sy;
    }
  }
}

void ssd1306_hline(ssd1306_t *ssd, uint8_t x0, uint8_t x1, uint8_t y, bool value)
{
  for (uint8_t x = x0; x <= x1; ++x)
    ssd1306_pixel(ssd, x, y, value);
}

void ssd1306_vline(ssd1306_t *ssd, uint8_t x, uint8_t y0, uint8_t y1, bool value)
{
  for (uint8_t y = y0; y <= y1; ++y)
    ssd1306_pixel(ssd, x, y, value);
}

// Função para desenhar um caractere
void ssd1306_draw_char(ssd1306_t *ssd, char c, uint8_t x, uint8_t y)
{
  uint16_t index = 0;
  char ver = c;
  if (c >= 'A' && c <= 'Z')
  {
    index = (c - 'A' + 11) * 8; // Para letras maiúsculas
  }
  else if (c >= '0' && c <= '9')
  {
    index = (c - '0' + 1) * 8; // Adiciona o deslocamento necessário
  }
  else if (c >= 'a' && c <= 'z')
  {
    index = (c - 'a' + 37) * 8;
  }
  for (uint8_t i = 0; i < 8; ++i)
  {
    uint8_t line = font[index + i];
    for (uint8_t j = 0; j < 8; ++j)
    {
      ssd1306_pixel(ssd, x + i, y + j, line & (1 << j));
    }
  }
}

void draw_square(ssd1306_t *display, int x, int y) {
    ssd1306_rect(display, y, x, SQUARE_SIZE, SQUARE_SIZE, true, true);
}

// Desenha borda estilizada no display (0-5 estilos)
void draw_border(ssd1306_t *display, uint8_t style)
{
  switch (style % 6)
  {       // 6 estilos diferentes
  case 0: // Estilo minimalista
    ssd1306_rect(display, 0, 0, WIDTH, HEIGHT, true, false);
    break;

  case 1: // Moldura dupla moderna
    ssd1306_rect(display, 0, 0, WIDTH, HEIGHT, true, false);
    ssd1306_rect(display, 3, 3, WIDTH - 6, HEIGHT - 6, true, false);
    break;

  case 2: // Cantos estilizados
    // Linhas principais
    ssd1306_rect(display, 4, 4, WIDTH - 8, HEIGHT - 8, true, false);

    // Detalhes decorativos nos cantos
    ssd1306_line(display, 0, 0, 7, 0, true);                                                                   // Topo esquerdo
    ssd1306_line(display, 0, 0, 0, 7, true);                                                                   // Lateral esquerda
    ssd1306_line(display, WIDTH - 1, 0, WIDTH - 8, 0, true);                                   // Topo direito
    ssd1306_line(display, WIDTH - 1, 0, WIDTH - 1, 7, true);                                   // Lateral direita
    ssd1306_line(display, 0, HEIGHT - 1, 7, HEIGHT - 1, true);                                 // Base esquerda
    ssd1306_line(display, 0, HEIGHT - 8, 0, HEIGHT - 1, true);                                 // Lateral esquerda
    ssd1306_line(display, WIDTH - 1, HEIGHT - 1, WIDTH - 8, HEIGHT - 1, true); // Base direita
    ssd1306_line(display, WIDTH - 1, HEIGHT - 8, WIDTH - 1, HEIGHT - 1, true); // Lateral direita
    break;

  case 3: // Efeito tridimensional
    // Sombras
    ssd1306_line(display, 1, 1, WIDTH - 2, 1, true);  // Topo
    ssd1306_line(display, 1, 1, 1, HEIGHT - 2, true); // Lateral

    // Realces
    ssd1306_line(display, WIDTH - 1, 1, WIDTH - 1, HEIGHT - 1, false);  // Lateral direita
    ssd1306_line(display, 1, HEIGHT - 1, WIDTH - 1, HEIGHT - 1, false); // Base

    // Moldura principal
    ssd1306_rect(display, 0, 0, WIDTH, HEIGHT, true, false);
    break;

  case 4: // Cantos arredondados
    // Linhas principais
    ssd1306_line(display, 3, 0, WIDTH - 4, 0, true);                                   // Topo
    ssd1306_line(display, 0, 3, 0, HEIGHT - 4, true);                                  // Lateral esquerda
    ssd1306_line(display, WIDTH - 1, 3, WIDTH - 1, HEIGHT - 4, true);  // Lateral direita
    ssd1306_line(display, 3, HEIGHT - 1, WIDTH - 4, HEIGHT - 1, true); // Base

    // Arredondamento dos cantos
    const uint8_t corners[12][2] = {{1, 1}, {0, 2}, {2, 0}, // Canto superior esquerdo
                                    {WIDTH - 2, 0},
                                    {WIDTH - 1, 1},
                                    {WIDTH - 3, 0}, // Canto superior direito
                                    {WIDTH - 1, HEIGHT - 2},
                                    {WIDTH - 2, HEIGHT - 1},
                                    {WIDTH - 3, HEIGHT - 1}, // Canto inferior direito
                                    {1, HEIGHT - 1},
                                    {0, HEIGHT - 2},
                                    {2, HEIGHT - 1}}; // Canto inferior esquerdo

    for (uint8_t i = 0; i < 12; i++)
    {
      ssd1306_pixel(display, corners[i][0], corners[i][1], true);
    }
    break;

  case 5: // Efeito de profundidade
    for (uint8_t i = 0; i < 3; i++)
    {
      ssd1306_rect(display, i * 2, i * 2, WIDTH - (i * 4), HEIGHT - (i * 4), true, false);
    }
    break;
  }
}

// Função para desenhar uma string
void ssd1306_draw_string(ssd1306_t *ssd, const char *str, uint8_t x, uint8_t y)
{
  while (*str)
  {
    ssd1306_draw_char(ssd, *str++, x, y);
    x += 8;
    if (x + 8 >= ssd->width)
    {
      x = 0;
      y += 8;
    }
    if (y + 8 >= ssd->height)
    {
      break;
    }
  }
}


void ssd1306_draw_bitmap(ssd1306_t *ssd, uint8_t x, uint8_t y, const uint8_t *bitmap, uint8_t width, uint8_t height) {
    // Calcula a página inicial e o número de páginas do bitmap
    uint8_t start_page = y / 8;
    uint8_t num_pages = height / 8;
    
    // Ajusta o número de páginas se exceder o limite do display
    if (start_page + num_pages > ssd->pages) {
        num_pages = ssd->pages - start_page;
    }

    // Itera sobre cada coluna do bitmap
    for (uint8_t x_offset = 0; x_offset < width; x_offset++) {
        uint8_t current_x = x + x_offset;
        // Verifica se a coluna atual está dentro dos limites do display
        if (current_x >= ssd->width) break;

        // Itera sobre cada página do bitmap
        for (uint8_t page = 0; page < num_pages; page++) {
            uint8_t current_page = start_page + page;
            // Verifica se a página atual está dentro dos limites
            if (current_page >= ssd->pages) break;

            // Obtém o byte correspondente do bitmap
            uint16_t bitmap_index = x_offset + page * width;
            uint8_t byte = bitmap[bitmap_index];

            // Calcula a posição no buffer do display
            uint16_t buffer_index = 1 + current_x + (current_page * ssd->width);
            
            // Atualiza o buffer apenas se o índice for válido
            if (buffer_index < ssd->bufsize) {
                ssd->ram_buffer[buffer_index] = byte;
            }
        }
    }
}