#include <gd.h>
#include <math.h>
#include <shapefil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define PIXEL_NORMAL 1
#define PIXEL_HIGHLIGHT 2
#define PIXEL_DARK 3
#define PIXEL_SUN 4
#define PIXEL_SUN_BORDER 5
#define PIXEL_LINE 6

#define DEG_2_RAD (M_PI / 180.0)
#define RAD_2_DEG (180.0 / M_PI)

struct screen
{
    int width, height;
    char *data;

    int solid_land;
    int world_border;
    int brush_color;
    int disable_colors;
    double shade_steps_degree;

    void (* project)(struct screen *s, double lon, double lat, double *x, double *y);

    struct sun
    {
        int active;
        double lon, lat;
    } sun;
};

void
project_kavrayskiy(struct screen *s, double lon, double lat, double *x, double *y)
{
    double lonr = lon * DEG_2_RAD;
    double latr = lat * DEG_2_RAD;

    /* Actual projection. */
    *x = 3.0 / 2 * lonr * sqrt(1.0 / 3 - (latr / M_PI) * (latr / M_PI));
    *y = lat;

    /* Scale to our screen. */
    *x *= RAD_2_DEG;
    *x = (*x + 180) / 360 * s->width;
    *y = (180 - (*y + 90)) / 180 * s->height;
}

void
project_lambert(struct screen *s, double lon, double lat, double *x, double *y)
{
    /* Actual projection. */
    *x = lon;
    *y = sin(lat * DEG_2_RAD);

    /* Scale to our screen. */
    *x = (*x + 180) / 360 * s->width;
    *y *= 90;
    *y = (180 - (*y + 90)) / 180 * s->height;
}

void
project_equirect(struct screen *s, double lon, double lat, double *x, double *y)
{
    *x = (lon + 180) / 360 * s->width;
    *y = (180 - (lat + 90)) / 180 * s->height;
}

void
calc_sun(struct sun *sun)
{
    time_t t;
    struct tm utc;

    time(&t);
    gmtime_r(&t, &utc);

    /* See german notes in "sonnenstand.txt". */
    sun->lon = ((utc.tm_hour * 60 * 60 + utc.tm_min * 60 + utc.tm_sec) -
                (86400.0 / 2 + ((-0.171 * sin(0.0337 * (utc.tm_yday + 1) + 0.465) -
                 0.1299 * sin(0.01787 * (utc.tm_yday + 1) - 0.168)) * -3600))) *
               (-360.0 / 86400);
    sun->lat = 0.4095 * sin(0.016906 * ((utc.tm_yday + 1) - 80.086)) * RAD_2_DEG;
}

void
screen_init(struct screen *s)
{
    s->brush_color = PIXEL_NORMAL;
    s->project = project_equirect;
    s->sun.active = 0;
    s->solid_land = 0;
    s->world_border = 0;
    s->disable_colors = 0;
    s->shade_steps_degree = 2;  /* TODO: Make this an option? */
}

int
screen_init_data(struct screen *s, int width, int height)
{
    s->width = width;
    s->height = height;
    s->data = calloc(1, width * height);
    if (s->data == NULL)
    {
        fprintf(stderr, "Out of memory in screen_init()\n");
        return 0;
    }
    return 1;
}

void
print_color(struct screen *s, char *str)
{
    if (!s->disable_colors)
        printf("%s", str);
}

void
screen_show_interpreted(struct screen *s, int trailing_newline)
{
    int x, y, sun_found, is_line, glyph;
    char a, b, c, d;
    char *charset[] = {  " ",  ".",  ",",  "_",  "'",  "|",  "/",  "J",
                         "`", "\\",  "|",  "L", "\"",  "7",  "r",  "#" };

    for (y = 0; y < s->height - 1; y += 2)
    {
        for (x = 0; x < s->width - 1; x += 2)
        {
            a = s->data[y * s->width + x];
            b = s->data[y * s->width + x + 1];
            c = s->data[(y + 1) * s->width + x];
            d = s->data[(y + 1) * s->width + x + 1];

            if (a == PIXEL_HIGHLIGHT || b == PIXEL_HIGHLIGHT ||
                c == PIXEL_HIGHLIGHT || d == PIXEL_HIGHLIGHT)
            {
                print_color(s, "\033[31;1m");
                printf("X");
                print_color(s, "\033[0m");
            }
            else
            {
                sun_found = 0;

                if (s->sun.active)
                {
                    if (a == PIXEL_SUN || b == PIXEL_SUN ||
                        c == PIXEL_SUN || d == PIXEL_SUN)
                    {
                        sun_found = 1;
                        print_color(s, "\033[36m");
                        printf("S");
                        print_color(s, "\033[0m");
                    }
                    else if (a == PIXEL_SUN_BORDER || b == PIXEL_SUN_BORDER ||
                             c == PIXEL_SUN_BORDER || d == PIXEL_SUN_BORDER)
                        print_color(s, "\033[36m");
                    else if (a == PIXEL_DARK || b == PIXEL_DARK ||
                             c == PIXEL_DARK || d == PIXEL_DARK)
                        print_color(s, "\033[30;1m");
                    else
                        print_color(s, "\033[33;1m");
                }

                if (!sun_found)
                {
                    is_line = 0;

                    if (a == PIXEL_LINE || b == PIXEL_LINE ||
                        c == PIXEL_LINE || d == PIXEL_LINE)
                    {
                        is_line = 1;
                        print_color(s, "\033[0m\033[37m");
                    }

                    glyph = (!!a << 3) | (!!b << 2) | (!!c << 1) | !!d;
                    printf("%s", charset[glyph]);

                    if (s->sun.active || is_line)
                        print_color(s, "\033[0m");
                }
            }
        }
        if (trailing_newline || y + 1 < s->height - 1)
            printf("\n");
    }
}

void
screen_set_pixel(struct screen *s, int x, int y, char val)
{
    if (x >= 0 && y >= 0 && x < s->width && y < s->height)
        s->data[y * s->width + x] = val;
}

void
screen_draw_line(struct screen *s, int x1, int y1, int x2, int y2)
{
    /* TODO: Replace with libgd as well? We might then kick our custom
     * framebuffer ... */

    int x, y, t, dx, dy, incx, incy, pdx, pdy, es, el, err;

    dx = x2 - x1;
    dy = y2 - y1;

    incx = dx < 0 ? -1 : 1;
    incy = dy < 0 ? -1 : 1;

    dx = dx < 0 ? -dx : dx;
    dy = dy < 0 ? -dy : dy;

    if (dx > dy)
    {
        pdx = incx;
        pdy = 0;
        es = dy;
        el = dx;
    }
    else
    {
        pdx = 0;
        pdy = incy;
        es = dx;
        el = dy;
    }

    x = x1;
    y = y1;
    err = el / 2;
    screen_set_pixel(s, x, y, s->brush_color);

    for (t = 0; t < el; t++)
    {
        err -= es;
        if (err < 0)
        {
            err += el;
            x += incx;
            y += incy;
        }
        else
        {
            x += pdx;
            y += pdy;
        }
        screen_set_pixel(s, x, y, s->brush_color);
    }
}

void
screen_draw_line_projected(struct screen *s, double lon1, double lat1,
                           double lon2, double lat2)
{
    double x1, y1, x2, y2;

    (s->project)(s, lon1, lat1, &x1, &y1);
    (s->project)(s, lon2, lat2, &x2, &y2);

    if ((int)x1 == (int)x2 && (int)y1 == (int)y2)
        return;

    screen_draw_line(s, x1, y1, x2, y2);
}

int
screen_draw_map(struct screen *s, char *file)
{
    int ret = 1;
    int i, n, t, v, white, black;
    int x, y;
    double x1, y1;
    gdPoint *polypoints = NULL;
    SHPHandle h;
    SHPObject *o;
    gdImagePtr img;

    img = gdImageCreate(s->width, s->height);
    black = gdImageColorAllocate(img, 0, 0, 0);
    (void)black;  /* only needed to set background */
    white = gdImageColorAllocate(img, 255, 255, 255);

    h = SHPOpen(file, "rb");
    if (h == NULL)
    {
        fprintf(stderr, "Could not open shapefile\n");
        ret = 0;
        goto out;
    }

    SHPGetInfo(h, &n, &t, NULL, NULL);

    if (t != SHPT_POLYGON)
    {
        fprintf(stderr, "This is not a polygon file\n");
        ret = 0;
        goto cleanout;
    }

    for (i = 0; i < n; i++)
    {
        o = SHPReadObject(h, i);
        if (o == NULL)
        {
            fprintf(stderr, "Could not read object %d\n", i);
            ret = 0;
            goto cleanout;
        }

        if (o->nSHPType != SHPT_POLYGON)
        {
            fprintf(stderr, "Shape %d is not a polygon", i);
            ret = 0;
            goto cleanout;
        }

        if (polypoints != NULL)
            free(polypoints);
        polypoints = malloc(sizeof(gdPoint) * o->nVertices);
        for (v = 0; v < o->nVertices; v++)
        {
            (s->project)(s, o->padfX[v], o->padfY[v], &x1, &y1);
            polypoints[v].x = x1;
            polypoints[v].y = y1;
        }

        if (s->solid_land)
            gdImageFilledPolygon(img, polypoints, o->nVertices, white);
        else
            gdImagePolygon(img, polypoints, o->nVertices, white);

        SHPDestroyObject(o);
    }

    for (y = 0; y < s->height; y++)
        for (x = 0; x < s->width; x++)
            screen_set_pixel(s, x, y, gdImageGetPixel(img, x, y) ? s->brush_color : 0);

    gdImageDestroy(img);

cleanout:
    SHPClose(h);
out:
    return ret;
}

int
screen_mark_locations(struct screen *s, char *file)
{
    FILE *fp;
    int scanret = 0;
    double lat, lon, sx, sy;

    fp = fopen(file, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Could not open locations file\n");
        return 0;
    }

    while (1)
    {
        scanret = fscanf(fp, "%lf %lf\n", &lat, &lon);

        if (scanret == EOF)
            break;

        if (scanret == 2)
        {
            (s->project)(s, lon, lat, &sx, &sy);

            s->data[(int)sy * s->width + (int)sx] = PIXEL_HIGHLIGHT;
        }
    }

    fclose(fp);

    return 1;
}

void
screen_mark_sun(struct screen *s)
{
    double x, y;

    (s->project)(s, s->sun.lon, s->sun.lat, &x, &y);

    s->data[(int)y * s->width + (int)x] = PIXEL_SUN;
}

void
screen_mark_sun_border(struct screen *s)
{
    double phi_n, lambda_n;
    double phi1, lambda1, phi2, lambda2, iscaled;
    double iscaled_smooth = 20;
    int i, steps = 128, modif;

    s->brush_color = PIXEL_SUN_BORDER;

    /* Again, see german notes in "sonnenstand.txt". */

    phi_n = (s->sun.lat + 90) * DEG_2_RAD;
    lambda_n = s->sun.lon * DEG_2_RAD;

    /* Around March 20 and September 20, use the alternative
     * parametrisation where we iterate over phi instead of lambda. */
    if (s->sun.lat + 90 > 86 && s->sun.lat + 90 < 94)
        for (modif = -1; modif <= 1; modif += 2)
            for (i = 0; i < steps; i++)
            {
                /* Do not run iscaled from [0:1] linearly but create more
                 * points close to 0 and close 1. Just plot this in gnuplot
                 * to see what's going on here:
                 *
                 *   plot [0:1] 0.5 * (atan(32 * x - 16) / atan(16) + 1)
                 */
                iscaled = (double)i / steps;
                iscaled = 0.5 * (atan(iscaled_smooth * iscaled - 0.5 * iscaled_smooth)
                          / atan(0.5 * iscaled_smooth) + 1);
                /* Run phi from phi_n to -phi_n (or the other way round). */
                phi1 = iscaled * 2 * phi_n - phi_n;
                /* Here, modif takes care of running in positive or negative
                 * direction. Depending on that, we might have to flip
                 * lambda. */
                lambda1 = acos(tan(modif * phi1) / tan(phi_n)) + lambda_n;
                lambda1 = modif == 1 ? lambda1 : lambda1 - M_PI;

                /* Okay, now do that same thing but based on "i + 1" instead
                 * of just "i". */
                iscaled = (double)(i + 1) / steps;
                iscaled = 0.5 * (atan(iscaled_smooth * iscaled - 0.5 * iscaled_smooth)
                          / atan(0.5 * iscaled_smooth) + 1);
                phi2 = iscaled * 2 * phi_n - phi_n;
                lambda2 = acos(tan(modif * phi2) / tan(phi_n)) + lambda_n;
                lambda2 = modif == 1 ? lambda2 : lambda2 - M_PI;

                /* Around the top/bottom, we might get NaNs. Skip those. */
                if (isnan(lambda1) || isnan(lambda2))
                    continue;

                /* If they're both over the edge, then push them both. */
                if (lambda1 < -M_PI && lambda2 < -M_PI)
                {
                    lambda1 += 2 * M_PI;
                    lambda2 += 2 * M_PI;
                }
                else if (lambda1 > M_PI && lambda2 > M_PI)
                {
                    lambda1 -= 2 * M_PI;
                    lambda2 -= 2 * M_PI;
                }
                /* If only one of them is over the edge, then skip this
                 * line. The results in missing segments around the
                 * poles. This is okay, though, because we're missing
                 * segments in those areas anyway. */
                else if (!(lambda1 >= -M_PI && lambda1 <= M_PI &&
                           lambda2 >= -M_PI && lambda2 <= M_PI))
                    continue;

                screen_draw_line_projected(s,
                                           lambda1 * RAD_2_DEG,
                                           phi1 * RAD_2_DEG,
                                           lambda2 * RAD_2_DEG,
                                           phi2 * RAD_2_DEG);
            }
    /* But by default, we iterate over lambda which is much simpler and
     * more stable (except for those cases above). */
    else
        for (i = 0; i < steps; i++)
        {
            lambda1 = (double)i / steps * 2 * M_PI - M_PI;
            phi1 = atan(tan(phi_n) * cos(lambda1 - lambda_n));

            lambda2 = (double)(i + 1) / steps * 2 * M_PI - M_PI;
            phi2 = atan(tan(phi_n) * cos(lambda2 - lambda_n));

            screen_draw_line_projected(s,
                                       lambda1 * RAD_2_DEG,
                                       phi1 * RAD_2_DEG,
                                       lambda2 * RAD_2_DEG,
                                       phi2 * RAD_2_DEG);
        }
}

void
screen_shade_map(struct screen *s)
{
    int dark, normal, shade;
    int ix, iy;
    gdImagePtr img;
    double phi, lambda, phi_sun, lambda_sun, zeta, aspan;
    double x, y;
    gdPoint polypoints[4];

    /* Render small patches in the color of daylight or night. The
     * result is then used as an overlay for the existing map. We're
     * still calculating everything in spherical coordinates which saves
     * us the need for an inverse projection. */

    img = gdImageCreate(s->width, s->height);
    dark = gdImageColorAllocate(img, 0, 0, 0);
    normal = gdImageColorAllocate(img, 255, 255, 255);

    aspan = s->shade_steps_degree * DEG_2_RAD;

    lambda_sun = s->sun.lon * DEG_2_RAD;
    phi_sun = s->sun.lat * DEG_2_RAD;

    /* TODO: Center patches? Only matters for high resolutions... */
    for (lambda = -180 * DEG_2_RAD; lambda < 180 * DEG_2_RAD - aspan; lambda += aspan)
    {
        for (phi = -90 * DEG_2_RAD; phi < 90 * DEG_2_RAD - aspan; phi += aspan)
        {
            /* Use the Great Circle Distance to determine color of this
             * patch. */
            zeta = acos(sin(phi_sun) * sin(phi) +
                    cos(phi_sun) * cos(phi) * cos(lambda - lambda_sun));

            if (zeta > 90 * DEG_2_RAD)
                shade = dark;
            else
                shade = normal;

            (s->project)(s, lambda * RAD_2_DEG, phi * RAD_2_DEG, &x, &y);
            polypoints[0].x = x;
            polypoints[0].y = y;
            (s->project)(s, (lambda + aspan) * RAD_2_DEG, phi * RAD_2_DEG, &x, &y);
            polypoints[1].x = x;
            polypoints[1].y = y;
            (s->project)(s, (lambda + aspan) * RAD_2_DEG, (phi + aspan) * RAD_2_DEG, &x, &y);
            polypoints[2].x = x;
            polypoints[2].y = y;
            (s->project)(s, lambda * RAD_2_DEG, (phi + aspan) * RAD_2_DEG, &x, &y);
            polypoints[3].x = x;
            polypoints[3].y = y;

            gdImageFilledPolygon(img, polypoints, 4, shade);
        }
    }

    /* XXX Hack: PIXEL_NORMAL is 1, PIXEL_DARK is 3. */
    for (iy = 0; iy < s->height; iy++)
        for (ix = 0; ix < s->width; ix++)
            if (gdImageGetPixel(img, ix, iy) == dark)
                s->data[iy * s->width + ix] *= 3;

    gdImageDestroy(img);
}

void
screen_draw_world_border(struct screen *s)
{
    int i, steps = 128;

    s->brush_color = PIXEL_LINE;

    for (i = 0; i < steps; i++)
    {
        screen_draw_line_projected(s,
                                   -179.99999,
                                   (double)i / steps * 180 - 90,
                                   -179.99999,
                                   (double)(i + 1) / steps * 180 - 90);
    }

    for (i = 0; i < steps; i++)
    {
        screen_draw_line_projected(s,
                                   179.99999,
                                   (double)i / steps * 180 - 90,
                                   179.99999,
                                   (double)(i + 1) / steps * 180 - 90);
    }

    for (i = 0; i < steps; i++)
    {
        screen_draw_line_projected(s,
                                   (double)i / steps * 360 - 180,
                                   -89.99999,
                                   (double)(i + 1) / steps * 360 - 180,
                                   -89.99999);
    }

    for (i = 0; i < steps; i++)
    {
        screen_draw_line_projected(s,
                                   (double)i / steps * 360 - 180,
                                   89.99999,
                                   (double)(i + 1) / steps * 360 - 180,
                                   89.99999);
    }
}

int
main(int argc, char **argv)
{
    struct screen s;
    struct winsize w;
    int trailing_newline = 1;
    int opt;
    char *map = "ne_110m_land.shp";
    char *highlight_locations = NULL;

    if (isatty(STDOUT_FILENO))
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    else
    {
        w.ws_col = 80;
        w.ws_row = 24;
    }

    screen_init(&s);

    while ((opt = getopt(argc, argv, "w:h:m:l:sTp:bCo")) != -1)
    {
        switch (opt)
        {
            case 'w':
                w.ws_col = atoi(optarg);
                break;
            case 'h':
                w.ws_row = atoi(optarg);
                break;
            case 'm':
                map = optarg;
                break;
            case 'l':
                highlight_locations = optarg;
                break;
            case 's':
                s.sun.active = 1;
                break;
            case 'T':
                trailing_newline = 0;
                break;
            case 'p':
                if (strncmp(optarg, "kav", 3) == 0)
                    s.project = project_kavrayskiy;
                else if (strncmp(optarg, "lam", 3) == 0)
                    s.project = project_lambert;
                break;
            case 'b':
                s.world_border = 1;
                break;
            case 'C':
                s.disable_colors = 1;
                break;
            case 'o':
                s.solid_land = 1;
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    if (s.sun.active)
        calc_sun(&s.sun);
    if (!screen_init_data(&s, 2 * w.ws_col, 2 * w.ws_row))
        exit(EXIT_FAILURE);
    if (!screen_draw_map(&s, map))
        exit(EXIT_FAILURE);
    if (s.sun.active)
    {
        screen_shade_map(&s);
        screen_mark_sun_border(&s);
    }
    if (s.world_border)
        screen_draw_world_border(&s);
    if (highlight_locations != NULL)
        if (!screen_mark_locations(&s, highlight_locations))
            exit(EXIT_FAILURE);
    if (s.sun.active)
        screen_mark_sun(&s);
    screen_show_interpreted(&s, trailing_newline);

    exit(EXIT_SUCCESS);
}
