#include <application.h>
#include <twr_fuel_cell_module.h>

#define TEMPERATURE_UPDATE_INTERVAL (5 * 1000)
#define VOLTAGE_UPDATE_INTERVAL (5 * 1000)

#define BATTERY_UPDATE_INTERVAL (1 * 60 * 1000)
#define APPLICATION_TASK_ID 0

#define TEMPERATURE_GRAPH (5 * 60 * 1000)
#define VOLTAGE_GRAPH (5 * 60 * 1000)


TWR_DATA_STREAM_FLOAT_BUFFER(temperature_stream_buffer, (TEMPERATURE_GRAPH / TEMPERATURE_UPDATE_INTERVAL))
TWR_DATA_STREAM_FLOAT_BUFFER(voltage_stream_buffer, (VOLTAGE_GRAPH / VOLTAGE_UPDATE_INTERVAL))


// LED instance
twr_led_t led;

// Button instance
twr_button_t button;

twr_tmp112_t tmp112;

twr_gfx_t *gfx;

float temperature = NAN;
float voltage = NAN;
float x = 0;

twr_data_stream_t temperature_stream;
twr_data_stream_t voltage_stream;


bool page = true;

/*static bool fuel_cell_meassure(float *voltage)
{
    *voltage = NAN;

    if (!twr_i2c_memory_write_16b(TWR_I2C_I2C0, 0x4B, 0x01, 0xc583))
        return false;

    int16_t result;

    if (!twr_i2c_memory_read_16b(TWR_I2C_I2C0, 0x4B, 0x00, (uint16_t *) &result))
        return false;

    if (result == 0x7ff0)
        return false;


    *voltage = result < 0 ? 0 : (uint64_t) (result >> 4) * 2048 * (10 + 5) / (2047 * 5);
    *voltage /= 1000.f;
    twr_log_debug("%d", *voltage);

    return true;
}*/

void button_event_handler(twr_button_t *self, twr_button_event_t event, void *event_param)
{
    if (event == TWR_BUTTON_EVENT_PRESS)
    {
        /*page = !page;*/

        twr_scheduler_plan_now(APPLICATION_TASK_ID);
    }
}

void lcd_event_handler(twr_module_lcd_event_t event, void *event_param)
{
    if(event == TWR_MODULE_LCD_EVENT_RIGHT_PRESS)
    {
        //x = ((float)rand()/(float)(RAND_MAX)) * 1.5f;

        x += 0.02;

        if(x > 1.14f)
        {
            x = 1.14f;
        }

        float y = pow(x, 2);

        twr_data_stream_feed(&temperature_stream, &y);
    }
    else if(event == TWR_MODULE_LCD_EVENT_LEFT_PRESS)
    {
        x -= 0.02;

        if(x < 0)
        {
            x = 0;
        }

        float y = pow(x, 2);

        twr_data_stream_feed(&temperature_stream, &y);
    }
}

void battery_event_handler(twr_module_battery_event_t event, void *event_param)
{
    (void) event;
    (void) event_param;

    voltage = NAN;

    if (twr_module_battery_get_voltage(&voltage))
    {
        twr_radio_pub_battery(&voltage);
    }
}

void tmp112_event_handler(twr_tmp112_t *self, twr_tmp112_event_t event, void *event_param)
{
    temperature = NAN;

    if (event == TWR_TMP112_EVENT_UPDATE)
    {
        if (twr_tmp112_get_temperature_celsius(self, &temperature))
        {
            /*twr_data_stream_feed(&temperature_stream, &temperature);

            twr_radio_pub_temperature(TWR_RADIO_PUB_CHANNEL_R1_I2C0_ADDRESS_DEFAULT, &temperature);

            float avg_temperature = NAN;

            twr_data_stream_get_average(&temperature_stream, &avg_temperature);*/
        }
    }

    twr_scheduler_plan_now(APPLICATION_TASK_ID);
}

void fuel_cell_module_event_handler(twr_module_fuel_cell_event_t event, void *param)
{
    if(event == TWR_MODULE_FUEL_CELL_EVENT_VOLTAGE)
    {
        voltage = NAN;
        twr_module_fuel_cell_get_voltage(&voltage);

        twr_log_debug("voltage: %.2f", voltage);

        twr_data_stream_feed(&voltage_stream, &voltage);

    }
    twr_scheduler_plan_now(APPLICATION_TASK_ID);

}

void application_init(void)
{
    twr_log_init(TWR_LOG_LEVEL_DUMP, TWR_LOG_TIMESTAMP_ABS);

    // Initialize LED
    twr_led_init(&led, TWR_GPIO_LED, false, false);
    twr_led_set_mode(&led, TWR_LED_MODE_OFF);

    // Initialize button
    twr_button_init(&button, TWR_GPIO_BUTTON, TWR_GPIO_PULL_DOWN, false);
    twr_button_set_event_handler(&button, button_event_handler, NULL);

    // Initialize battery
    twr_module_battery_init();
    twr_module_battery_set_event_handler(battery_event_handler, NULL);
    twr_module_battery_set_update_interval(BATTERY_UPDATE_INTERVAL);

    twr_tmp112_init(&tmp112, TWR_I2C_I2C0, 0x49);
    twr_tmp112_set_event_handler(&tmp112, tmp112_event_handler, NULL);
    twr_tmp112_set_update_interval(&tmp112, TEMPERATURE_UPDATE_INTERVAL);

    twr_data_stream_init(&temperature_stream, 1, &temperature_stream_buffer);
    twr_data_stream_init(&voltage_stream, 1, &voltage_stream_buffer);

    twr_radio_init(TWR_RADIO_MODE_NODE_SLEEPING);
    twr_radio_pairing_request("voc-sensor", VERSION);

    twr_module_lcd_init();
    twr_module_lcd_set_event_handler(lcd_event_handler, NULL);
    gfx = twr_module_lcd_get_gfx();


    twr_module_fuel_cell_init();
    twr_module_fuel_cell_set_event_handler(fuel_cell_module_event_handler, NULL);
    twr_module_fuel_cell_set_update_interval(VOLTAGE_UPDATE_INTERVAL);

    twr_led_pulse(&led, 2000);


}

void graph(twr_gfx_t *gfx, int x0, int y0, int x1, int y1, twr_data_stream_t *data_stream, int time_step, float min_value, float max_value, int number_of_y_parts, bool grid_lines, const char *format)
{
    int w, h;
    char str[32];
    int width = x1 - x0;
    int height = y1 - y0;

    /*twr_data_stream_get_max(data_stream, &max_value);

    twr_data_stream_get_min(data_stream, &min_value);*/

    /*if (min_value > 0)
    {
        min_value = 0;
    }*/

    //max_value = ceilf(max_value / 5) * 5;

    twr_module_lcd_set_font(&twr_font_ubuntu_11);

    int number_of_samples = twr_data_stream_get_number_of_samples(data_stream);

    int end_time = - number_of_samples * time_step / 1000;

    h = 10;

    int first_line_x = 40;

    float range = fabsf(max_value) + fabsf(min_value);
    float fh = height - h - 2;

    snprintf(str, sizeof(str), "%ds", end_time);
    w = twr_gfx_calc_string_width(gfx, str) + 8;

    int lines = width / w;
    int y_time = y1 - h - 2;
    int y_zero = range > 0 ? y_time - ((fabsf(min_value) / range) * fh) : y_time;
    int tmp;

    for (int i = 0, time_step = end_time / lines, w_step = width / lines; i < lines; i++)
    {
        snprintf(str, sizeof(str), "%ds", time_step * i);

        w = twr_gfx_calc_string_width(gfx, str);

        tmp = width - w_step * i;
        first_line_x = tmp;

        twr_gfx_draw_string(gfx, tmp - w, y1 - h, str, 1);

        twr_gfx_draw_line(gfx, tmp - 3, y_zero - 2, tmp - 3, y_zero + 2, 1);
        twr_gfx_draw_line(gfx, tmp - 2, y_zero - 2, tmp - 2, y_zero + 2, 1);
        twr_gfx_draw_line(gfx, tmp - 1, y_zero - 2, tmp - 1, y_zero + 2, 1);

        twr_gfx_draw_line(gfx, tmp - 3, y0, tmp - 3, y0 + 2, 1);
        twr_gfx_draw_line(gfx, tmp - 2, y0, tmp - 2, y0 + 2, 1);
        twr_gfx_draw_line(gfx, tmp - 1, y0, tmp - 1, y0 + 2, 1);


        if(grid_lines)
        {
            twr_gfx_draw_line(gfx, tmp - 2, y_zero - 2, tmp - 2, y0, 1);
        }

        if (y_time != y_zero)
        {
            twr_gfx_draw_line(gfx, tmp - 2, y_time - 2, tmp - 2, y_time, 1);
        }
    }

    twr_gfx_draw_line(gfx, x0, y_zero, x1, y_zero, 1);

    if (y_time != y_zero)
    {
        twr_gfx_draw_line(gfx, x0, y_time, y1, y_time, 1);

        snprintf(str, sizeof(str), format, min_value);

        twr_gfx_draw_string(gfx, x0, y_time - 10, str, 1);
    }

    twr_gfx_draw_line(gfx, x0, y0, x1, y0, 1);

    snprintf(str, sizeof(str), format, max_value);

    twr_gfx_draw_string(gfx, x0, y0, str, 1);

    twr_gfx_draw_string(gfx, x0, y_zero - 10, "0", 1);

    int step = (y_zero - y0) / (number_of_y_parts - 1);
    float step_number = (max_value - min_value) / (float)(number_of_y_parts - 1);

    for(int i = 1; i < number_of_y_parts - 1; i++)
    {
        twr_gfx_draw_line(gfx, x0, (y_zero - step * i) - 1, x0 + 2, (y_zero - step * i) - 1, 1);
        twr_gfx_draw_line(gfx, x0, (y_zero - step * i), x0 + 2, (y_zero - step * i), 1);
        twr_gfx_draw_line(gfx, x0, (y_zero - step * i) + 1, x0 + 2, (y_zero - step * i) + 1, 1);

        twr_gfx_printf(gfx, x0, (y_zero - step * i), 1, "%.2f", step_number * i);

        if(grid_lines)
        {
            twr_gfx_draw_line(gfx, x0, (y_zero - step * i), x1, (y_zero - step * i), 1);
        }
    }

    if (range == 0)
    {
        return;
    }

    int length = twr_data_stream_get_length(data_stream);
    float value;

    int x_zero = x1 - 2;
    float fy;

    int dx = width / (number_of_samples - 1);
    int point_x = x_zero + dx;
    int point_y;
    int last_x;
    int last_y;

    min_value = fabsf(min_value);

    for (int i = 1; i <= length; i++)
    {
        if (twr_data_stream_get_nth(data_stream, -i, &value))
        {
            fy = (value + min_value) / range;

            point_y = y_time - (fy * fh);
            point_x -= dx;

            if (i == 1)
            {
                last_y = point_y;
                last_x = point_x;
            }

            if(point_x > first_line_x)
            {
                twr_gfx_draw_fill_rectangle(gfx, point_x - 1, point_y - 1, point_x + 1, point_y + 1, 1);

                twr_gfx_draw_line(gfx, point_x, point_y, last_x, last_y, 1);
            }

            last_y = point_y;
            last_x = point_x;

        }
    }
}

void application_task(void)
{
    twr_adc_async_measure(TWR_ADC_CHANNEL_A0);

    if (!twr_gfx_display_is_ready(gfx))
    {
        return;
    }

    twr_system_pll_enable();

    twr_gfx_clear(gfx);

    if (page)
    {

        int w;
        twr_gfx_set_font(gfx, &twr_font_ubuntu_15);
        w = twr_gfx_draw_string(gfx, 5, 23, "Charge", 1);

        twr_gfx_set_font(gfx, &twr_font_ubuntu_28);
        w = twr_gfx_printf(gfx, w + 5, 15, 1, "%.2f", x);

        twr_gfx_set_font(gfx, &twr_font_ubuntu_15);
        twr_gfx_draw_string(gfx, w + 5, 23, "V", 1);

        graph(gfx, 0, 40, 127, 127, &temperature_stream, TEMPERATURE_UPDATE_INTERVAL, 0, 1.5f, 4, true, "%.2f" "\xb0" "V");

    }
    twr_gfx_update(gfx);

    twr_system_pll_disable();
}


