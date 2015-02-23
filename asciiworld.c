#include <gd.h>
#include <math.h>
#include <shapefil.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

#define DEG_2_RAD (M_PI / 180.0)
#define RAD_2_DEG (180.0 / M_PI)

#define NUM_SHADES 8
#define NUM_TRACKS 3

enum sequence { SEQ_RESET, SEQ_LOCATION, SEQ_SUN, SEQ_SUN_BORDER, SEQ_SHADE1,
                SEQ_SHADE2, SEQ_SHADE3, SEQ_SHADE4, SEQ_SHADE5, SEQ_SHADE6,
                SEQ_SHADE7, SEQ_SHADE8, SEQ_LINE, SEQ_TRACK1, SEQ_TRACK2,
                SEQ_TRACK3, SEQ_TITLE };
char *seq_256colors[] = { "\033[0m",               /* reset */
                          "\033[38;5;196m",        /* location */
                          "\033[38;5;220m",        /* sun */
                          "\033[38;5;220m",        /* sun border */
                          "\033[38;5;18m",         /* shade1 */
                          "\033[38;5;19m",         /* shade2 */
                          "\033[38;5;21m",         /* shade3 */
                          "\033[38;5;26m",         /* shade4 */
                          "\033[38;5;30m",         /* shade5 */
                          "\033[38;5;35m",         /* shade6 */
                          "\033[38;5;40m",         /* shade7 */
                          "\033[38;5;46m",         /* shade8 */
                          "\033[38;5;255m",        /* line */
                          "\033[38;5;201m",        /* track1 */
                          "\033[38;5;255m",        /* track2 */
                          "\033[38;5;202m",        /* track3 */
                          "\033[1m" };             /* title */
char *seq_8colors[] = { "\033[0m",                 /* reset */
                        "\033[31;1m",              /* location */
                        "\033[33m",                /* sun */
                        "\033[36m",                /* sun border */
                        "\033[34m",                /* shade1 */
                        "\033[34m",                /* shade2 */
                        "\033[34;1m",              /* shade3 */
                        "\033[34;1m",              /* shade4 */
                        "\033[32m",                /* shade5 */
                        "\033[32m",                /* shade6 */
                        "\033[32;1m",              /* shade7 */
                        "\033[32;1m",              /* shade8 */
                        "\033[37m",                /* line */
                        "\033[35;1m",              /* track1 */
                        "\033[35;1m",              /* track2 */
                        "\033[35;1m",              /* track3 */
                        "\033[1m" };               /* title */

struct screen
{
    int width, height;
    gdImagePtr img;
    int col_black;
    int col_normal;
    int col_shade[NUM_SHADES];
    int col_track[NUM_TRACKS];
    int col_highlight;
    int col_sun;
    int col_sun_border;
    int col_line;
    int brush;

    char **esc_seq;
    char *title;

    int solid_land;
    int world_border;
    int disable_colors;
    double shade_steps_degree;
    double dusk_degree;

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
project_hammer(struct screen *s, double lon, double lat, double *x, double *y)
{
    double lonr = lon * DEG_2_RAD;
    double latr = lat * DEG_2_RAD;

    /* Actual projection. */
    *x = (2 * sqrt(2) * cos(latr) * sin(lonr * 0.5)) /
         sqrt(1 + cos(latr) * cos(lonr * 0.5));
    *y = (sqrt(2) * sin(latr)) /
         sqrt(1 + cos(latr) * cos(lonr * 0.5));

    /* Scale to our screen. */
    *x *= RAD_2_DEG;
    *y *= RAD_2_DEG;
    *x = (*x + 180) / 360 * s->width;
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
    s->project = project_equirect;
    s->sun.active = 0;
    s->solid_land = 0;
    s->world_border = 0;
    s->disable_colors = 0;
    s->shade_steps_degree = 1;
    s->dusk_degree = 6;
    s->esc_seq = seq_256colors;
    s->title = NULL;
}

int
screen_init_img(struct screen *s, int width, int height)
{
    int i;

    s->width = width;
    s->height = height;

    s->img = gdImageCreate(s->width, s->height);
    if (s->img == NULL)
    {
        fprintf(stderr, "Could not create image of size %dx%d.\n",
                s->width, s->height);
        return 0;
    }

    /* For ASCII output, these colors have no meaning. They're only
     * relevant if you're saving as PNG. Internally, we only use the
     * integers that libgd assigns to each color. */
    s->col_black = gdImageColorAllocate(s->img, 0, 0, 0);
    s->col_normal = gdImageColorAllocate(s->img, 200, 200, 200);
    for (i = 0; i < NUM_SHADES; i++)
    {
        s->col_shade[i] = gdImageColorAllocate(s->img, 0,
                                               255 * i / (double)(NUM_SHADES - 1),
                                               255 * (NUM_SHADES - 1 - i) /
                                                (double)(NUM_SHADES - 1));
    }
    for (i = 0; i < NUM_TRACKS; i++)
    {
        s->col_track[i] = gdImageColorAllocate(s->img, 255, 0,
                                               255 * (NUM_TRACKS - 1 - i) /
                                                (double)(NUM_TRACKS - 1));
    }
    s->col_highlight = gdImageColorAllocate(s->img, 255, 0, 0);
    s->col_sun = gdImageColorAllocate(s->img, 255, 255, 0);
    s->col_sun_border = gdImageColorAllocate(s->img, 255, 255, 0);
    s->col_line = gdImageColorAllocate(s->img, 255, 255, 255);

    return 1;
}

void
print_color(struct screen *s, enum sequence seq)
{
    if (!s->disable_colors)
        printf("%s", s->esc_seq[seq]);
}

int
screen_print_title(struct screen *s, int x, int y)
{
    int tx1, tx2, bx1, bx2, bmx1, bmx2, xr, yr, ti;
    const int box_x_margin = 2;

    if (s->title == NULL)
        return 0;

    tx1 = (s->width / 2 - strlen(s->title)) - 2 * box_x_margin;
    tx2 = tx1 + strlen(s->title);

    bx1 = tx1 - box_x_margin;
    bx2 = tx2 + box_x_margin;

    bmx1 = bx1 - box_x_margin;
    bmx2 = bx2 + box_x_margin;

    if (bmx1 < 0 || bmx2 > s->width / 2)
        return 0;

    xr = x / 2;
    yr = y / 2;

    ti = xr - tx1;

    if (bmx1 <= xr && xr < bmx2 && yr <= 4)
    {
        if (xr < bx1 || bx2 <= xr)
            printf(" ");
        else
        {
            switch (yr)
            {
                case 0:
                case 4:
                    printf(" ");
                    break;
                case 1:
                case 3:
                    if (xr == bx1 || xr == bx2 - 1)
                        printf("+");
                    else
                        printf("-");
                    break;
                case 2:
                    if (xr == bx1 || xr == bx2 - 1)
                        printf("|");
                    else
                    {
                        if (xr < tx1 || tx2 <= xr)
                            printf(" ");
                        else
                        {
                            print_color(s, SEQ_TITLE);
                            printf("%c", s->title[ti]);
                            print_color(s, SEQ_RESET);
                        }
                    }
                    break;
            }
        }
        return 1;
    }
    return 0;
}

void
screen_show_interpreted(struct screen *s, int trailing_newline)
{
    int x, y, i, sun_found, is_sun_border, is_track, is_line, glyph;
    int a, b, c, d;
    char *charset[] = {  " ",  ".",  ",",  "_",  "'",  "|",  "/",  "J",
                         "`", "\\",  "|",  "L", "\"",  "7",  "r",  "o" };
    char *char_location = "X";
    char *char_sun = "S";
    char *char_sun_border = ":";
    char *char_track = "O";

    for (y = 0; y < s->height - 1; y += 2)
    {
        for (x = 0; x < s->width - 1; x += 2)
        {
            if (screen_print_title(s, x, y))
                continue;

            a = gdImageGetPixel(s->img, x, y);
            b = gdImageGetPixel(s->img, x + 1, y);
            c = gdImageGetPixel(s->img, x, y + 1);
            d = gdImageGetPixel(s->img, x + 1, y + 1);

            if (a == s->col_highlight || b == s->col_highlight ||
                c == s->col_highlight || d == s->col_highlight)
            {
                print_color(s, SEQ_LOCATION);
                printf("%s", char_location);
                print_color(s, SEQ_RESET);
            }
            else
            {
                sun_found = 0;
                is_sun_border = 0;
                is_track = 0;

                for (i = 0; i < NUM_TRACKS; i++)
                    if (a == s->col_track[i] || b == s->col_track[i] ||
                        c == s->col_track[i] || d == s->col_track[i])
                    {
                        print_color(s, SEQ_TRACK1 + i);
                        is_track = 1;
                        break;
                    }

                if (s->sun.active)
                {
                    if (a == s->col_sun || b == s->col_sun ||
                        c == s->col_sun || d == s->col_sun)
                    {
                        sun_found = 1;
                        print_color(s, SEQ_SUN);
                        printf("%s", char_sun);
                        print_color(s, SEQ_RESET);
                    }
                    else if (a == s->col_sun_border || b == s->col_sun_border ||
                             c == s->col_sun_border || d == s->col_sun_border)
                    {
                        is_sun_border = 1;
                        print_color(s, SEQ_SUN_BORDER);
                    }
                    else if (!is_track)
                        for (i = 0; i < NUM_SHADES; i++)
                            if (a == s->col_shade[i] || b == s->col_shade[i] ||
                                c == s->col_shade[i] || d == s->col_shade[i])
                            {
                                print_color(s, SEQ_SHADE1 + i);
                                break;
                            }
                }

                if (!sun_found)
                {
                    is_line = 0;

                    if (a == s->col_line || b == s->col_line ||
                        c == s->col_line || d == s->col_line)
                    {
                        is_line = 1;
                        print_color(s, SEQ_RESET);
                        print_color(s, SEQ_LINE);
                    }

                    if (is_track)
                        printf("%s", char_track);
                    else if (is_sun_border)
                        printf("%s", char_sun_border);
                    else
                    {
                        glyph = (!!a << 3) | (!!b << 2) | (!!c << 1) | !!d;
                        printf("%s", charset[glyph]);
                    }

                    if (s->sun.active || is_line || is_track)
                        print_color(s, SEQ_RESET);
                }
            }
        }
        if (trailing_newline || y + 1 < s->height - 1)
            printf("\n");
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

    gdImageLine(s->img, x1, y1, x2, y2, s->brush);
}

void
screen_draw_spherical_circle(struct screen *s, double lon_deg,
                             double lat_deg, double r_deg)
{
    int i, steps = 1024;
    double slat_deg;
    double s_theta, s_phi;
    double rx, ry, rz;
    double px, py, pz;
    double p2x, p2y, p2z, p2z_fixed;
    double alpha, m[9];
    double x, y;

    /* Get latitude of one point on the small circle. We can easily do
     * this by adjusting the latitude but we have to avoid pushing over
     * a pole. */
    if (lat_deg > 0)
        slat_deg = lat_deg - r_deg;
    else
        slat_deg = lat_deg + r_deg;

    /* Geographic coordinates to spherical coordinates. */
    s_theta = -(lat_deg * DEG_2_RAD) + (90 * DEG_2_RAD);
    s_phi = lon_deg * DEG_2_RAD;

    /* Cartesian coordinates of rotation axis. */
    rx = sin(s_theta) * cos(s_phi);
    ry = sin(s_theta) * sin(s_phi);
    rz = cos(s_theta);

    /* Rotation matrix around r{x,y,z} by alpha. */
    alpha = (360.0 / steps) * DEG_2_RAD;

    m[0] = rx * rx * (1 - cos(alpha)) + cos(alpha);
    m[1] = ry * rx * (1 - cos(alpha)) + rz * sin(alpha);
    m[2] = rz * rx * (1 - cos(alpha)) - ry * sin(alpha);

    m[3] = rx * ry * (1 - cos(alpha)) - rz * sin(alpha);
    m[4] = ry * ry * (1 - cos(alpha)) + cos(alpha);
    m[5] = rz * ry * (1 - cos(alpha)) + rx * sin(alpha);

    m[6] = rx * rz * (1 - cos(alpha)) + ry * sin(alpha);
    m[7] = ry * rz * (1 - cos(alpha)) - rx * sin(alpha);
    m[8] = rz * rz * (1 - cos(alpha)) + cos(alpha);

    /* Cartesian coordinates of initial vector. */
    s_theta = -(slat_deg * DEG_2_RAD) + (90 * DEG_2_RAD);
    s_phi = lon_deg * DEG_2_RAD;
    px = sin(s_theta) * cos(s_phi);
    py = sin(s_theta) * sin(s_phi);
    pz = cos(s_theta);

    for (i = 0; i < steps; i++)
    {
        /* Rotate p{x,y,z}. */
        p2x = px * m[0] + py * m[3] + pz * m[6];
        p2y = px * m[1] + py * m[4] + pz * m[7];
        p2z = px * m[2] + py * m[5] + pz * m[8];

        /* For acos(), force p2z back into [-1, 1] which *might* happen
         * due to precision errors. */
        p2z_fixed = fmax(-1, fmin(1, p2z));

        /* Convert back to spherical coordinates and then geographic
         * coordinates. */
        s_theta = acos(p2z_fixed);
        s_phi = atan2(p2y, p2x);

        lat_deg = ((90 * DEG_2_RAD) - s_theta) * RAD_2_DEG;
        lon_deg = s_phi * RAD_2_DEG;

        /* Project and draw pixel. */
        (s->project)(s, lon_deg, lat_deg, &x, &y);
        gdImageSetPixel(s->img, x, y, s->brush);

        /* Use rotated p{x,y,z} as basis for next rotation. */
        px = p2x;
        py = p2y;
        pz = p2z;
    }
}

int
poly_orientation(double *v)
{
    double e1[2], e2[2], z;

    e1[0] = v[2] - v[0];
    e1[1] = v[3] - v[1];

    e2[0] = v[4] - v[2];
    e2[1] = v[5] - v[3];

    z = e1[0] * e2[1] - e1[1] * e2[0];

    return z > 0 ? 1 : -1;
}

int
screen_draw_map(struct screen *s, char *file)
{
    int ret = 1;
    int i, n, t, v, p, vpoly, ori;
    double x1, y1;
    double vori[6] = { 0 };
    gdPoint *polypoints = NULL;
    SHPHandle h;
    SHPObject *o;

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

        v = 0;
        p = 0;
        vpoly = 0;
        ori = 0;

        while (v < o->nVertices)
        {
            if (p < o->nParts && v == o->panPartStart[p])
            {
                /* Finish previous part. */
                if (p != 0)
                {
                    if (s->solid_land)
                        gdImageFilledPolygon(s->img, polypoints, vpoly,
                                             ori > 0 && o->nParts > 1 ?
                                                s->col_black :
                                                s->col_normal);
                    else
                        gdImagePolygon(s->img, polypoints, vpoly, s->col_normal);
                }

                /* Start of part number "p" */
                p++;
                vpoly = 0;
                ori = 0;
            }

            (s->project)(s, o->padfX[v], o->padfY[v], &x1, &y1);
            polypoints[vpoly].x = x1;
            polypoints[vpoly].y = y1;

            /* We have to determine whether the points in this polygon
             * are ordered clockwise or counter-clockwise. This tells us
             * if this polygon is a "hole" (i.e., a lake). To do so, we
             * use Paul Bourke's test.
             *
             * Note that the resulting "ori" will only be used if this
             * polygon has more than one part. This is a hack to account
             * for slightly broken NED data. If we were to follow the
             * shapefile standard, we'd have to always use "ori". */
            if (vpoly < 3)
            {
                vori[2 * vpoly] = o->padfX[v];
                vori[2 * vpoly + 1] = o->padfY[v];
            }
            else
            {
                vori[0] = vori[2];
                vori[1] = vori[3];

                vori[2] = vori[4];
                vori[3] = vori[5];

                vori[4] = o->padfX[v];
                vori[5] = o->padfY[v];

                ori += poly_orientation(vori);
            }

            v++;
            vpoly++;
        }

        if (s->solid_land)
            gdImageFilledPolygon(s->img, polypoints, vpoly,
                                 ori > 0 && o->nParts > 1 ?
                                    s->col_black :
                                    s->col_normal);
        else
            gdImagePolygon(s->img, polypoints, vpoly, s->col_normal);

        SHPDestroyObject(o);
    }

cleanout:
    SHPClose(h);
out:
    return ret;
}

int
screen_mark_locations(struct screen *s, char *file)
{
    FILE *fp;
    char *line = NULL;
    size_t line_n = 0;
    int tracki = -1;
    double lat, lon, r, x, y;

    fp = fopen(file, "r");
    if (fp == NULL)
    {
        fprintf(stderr, "Could not open locations file\n");
        return 0;
    }

    /* Read block headers, tracks or circles. */
    while (getline(&line, &line_n, fp) != -1)
    {
        if (strncmp(line, "track\n", strlen("track\n")) == 0)
        {
            /* All points on a track have the same color. */
            tracki++;
            tracki %= NUM_TRACKS;
            s->brush = s->col_track[tracki];

            /* Read points until EOF or block delimiter. */
            while (getline(&line, &line_n, fp) != -1 &&
                   strncmp(line, ".\n", strlen(".\n")) != 0)
            {
                if (sscanf(line, "%lf %lf\n", &lat, &lon) == 2)
                {
                    (s->project)(s, lon, lat, &x, &y);
                    gdImageSetPixel(s->img, x, y, s->brush);
                }
            }
        }
        else if (strncmp(line, "circles\n", strlen("circles\n")) == 0)
        {
            /* Read circles until EOF or block delimiter. */
            while (getline(&line, &line_n, fp) != -1 &&
                   strncmp(line, ".\n", strlen(".\n")) != 0)
            {
                if (sscanf(line, "%lf %lf %lf\n", &lat, &lon, &r) == 3)
                {
                    /* Each circle has a new color. */
                    tracki++;
                    tracki %= NUM_TRACKS;
                    s->brush = s->col_track[tracki];

                    screen_draw_spherical_circle(s, lon, lat, r);
                }
            }
        }
    }

    /* Read again from start, but only points this time. */
    s->brush = s->col_highlight;
    fseek(fp, 0, SEEK_SET);
    while (getline(&line, &line_n, fp) != -1)
    {
        if (strncmp(line, "points", strlen("points")) == 0)
        {
            /* Read points until EOF or block delimiter. */
            while (getline(&line, &line_n, fp) != -1 &&
                   strncmp(line, ".\n", strlen(".\n")) != 0)
            {
                if (sscanf(line, "%lf %lf\n", &lat, &lon) == 2)
                {
                    (s->project)(s, lon, lat, &x, &y);
                    gdImageSetPixel(s->img, x, y, s->brush);
                }
            }
        }
    }

    free(line);
    fclose(fp);

    return 1;
}

void
screen_mark_sun(struct screen *s)
{
    double x, y;

    (s->project)(s, s->sun.lon, s->sun.lat, &x, &y);
    gdImageSetPixel(s->img, x, y, s->col_sun);
}

void
screen_mark_sun_border(struct screen *s)
{
    /* Again, see german notes in "sonnenstand.txt". */

    s->brush = s->col_sun_border;
    screen_draw_spherical_circle(s, s->sun.lon, s->sun.lat, 90);
}

void
screen_shade_map(struct screen *s)
{
    int black, shade_brush, orig;
    int shade[NUM_SHADES];
    int ix, iy, i, di90;
    gdImagePtr img;
    double phi, lambda, phi_sun, lambda_sun, zeta, aspan;
    double d90;
    double x, y;
    gdPoint polypoints[4];

    /* Render small patches in the color of daylight or night. The
     * result is then used as an overlay for the existing map. We're
     * still calculating everything in spherical coordinates which saves
     * us the need for an inverse projection. */

    img = gdImageCreate(s->width, s->height);
    black = gdImageColorAllocate(img, 0, 0, 0);
    (void)black;  /* only needed to set background */
    for (i = 0; i < NUM_SHADES; i++)
        shade[i] = gdImageColorAllocate(img, 0, 0, i);

    aspan = s->shade_steps_degree * DEG_2_RAD;

    lambda_sun = s->sun.lon * DEG_2_RAD;
    phi_sun = s->sun.lat * DEG_2_RAD;

    for (lambda = -180 * DEG_2_RAD; lambda < 180 * DEG_2_RAD - aspan; lambda += aspan)
    {
        for (phi = -90 * DEG_2_RAD; phi < 90 * DEG_2_RAD - aspan; phi += aspan)
        {
            /* Use the Great Circle Distance to determine color of this
             * patch. */
            zeta = acos(sin(phi_sun) * sin(phi) +
                    cos(phi_sun) * cos(phi) * cos(lambda - lambda_sun));

            /* It's daylight if the sun is above the horizon. If it's
             * below, we're in some kind of twilight (according to
             * dusk_degree) or night. */
            d90 = zeta * RAD_2_DEG - 90;
            d90 /= s->dusk_degree;
            di90 = (NUM_SHADES - 1) - (int)round(d90 * (NUM_SHADES - 1));
            di90 = di90 < 0 ? 0 : di90;
            di90 = di90 > NUM_SHADES - 1 ? NUM_SHADES - 1 : di90;
            shade_brush = shade[di90];

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

            gdImageFilledPolygon(img, polypoints, 4, shade_brush);
        }
    }

    /* Use img as overlay for s->img. Since these are two different
     * images with two different color spaces, we have to use "if". */
    for (iy = 0; iy < s->height; iy++)
    {
        for (ix = 0; ix < s->width; ix++)
        {
            orig = gdImageGetPixel(s->img, ix, iy);
            if (orig != s->col_black)
            {
                for (i = 0; i < NUM_SHADES; i++)
                {
                    if (gdImageGetPixel(img, ix, iy) == shade[i])
                    {
                        gdImageSetPixel(s->img, ix, iy, s->col_shade[i]);
                        break;
                    }
                }
            }
        }
    }

    gdImageDestroy(img);
}

void
screen_draw_world_border(struct screen *s)
{
    int i, steps = 128;

    s->brush = s->col_line;

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
    int trailing_newline = 1, sun_markers = 1;
    int opt, c;
    char *map = DEFAULT_MAP;
    char *highlight_locations = NULL;
    char *outimg = NULL;
    FILE *fp = NULL;

    if (isatty(STDOUT_FILENO))
        ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    else
    {
        w.ws_col = 80;
        w.ws_row = 24;
    }

    screen_init(&s);

    while ((opt = getopt(argc, argv, "w:h:m:l:sSTp:bc:od:W:t:")) != -1)
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
            case 'S':
                sun_markers = 0;
                break;
            case 'T':
                trailing_newline = 0;
                break;
            case 'p':
                if (strncmp(optarg, "kav", 3) == 0)
                    s.project = project_kavrayskiy;
                else if (strncmp(optarg, "lam", 3) == 0)
                    s.project = project_lambert;
                else if (strncmp(optarg, "ham", 3) == 0)
                    s.project = project_hammer;
                break;
            case 'b':
                s.world_border = 1;
                break;
            case 'c':
                c = atoi(optarg);
                if (c == 0)
                    s.disable_colors = 1;
                else if (c == 8)
                    s.esc_seq = seq_8colors;
                break;
            case 'o':
                s.solid_land = 1;
                break;
            case 'd':
                if (strncmp(optarg, "nau", 3) == 0)
                    s.dusk_degree = 12;
                else if (strncmp(optarg, "ast", 3) == 0)
                    s.dusk_degree = 18;
                break;
            case 'W':
                outimg = optarg;
                break;
            case 't':
                s.title = optarg;
                break;
            default:
                exit(EXIT_FAILURE);
        }
    }

    if (s.sun.active)
        calc_sun(&s.sun);
    if (!screen_init_img(&s, (outimg == NULL ? 2 : 1) * w.ws_col,
                         (outimg == NULL ? 2 : 1) * w.ws_row))
        exit(EXIT_FAILURE);
    if (!screen_draw_map(&s, map))
        exit(EXIT_FAILURE);
    if (s.sun.active)
    {
        screen_shade_map(&s);
        if (sun_markers)
            screen_mark_sun_border(&s);
    }
    if (s.world_border)
        screen_draw_world_border(&s);
    if (highlight_locations != NULL)
        if (!screen_mark_locations(&s, highlight_locations))
            exit(EXIT_FAILURE);
    if (s.sun.active && sun_markers)
        screen_mark_sun(&s);
    if (outimg == NULL)
        screen_show_interpreted(&s, trailing_newline);
    else
    {
        fp = fopen(outimg, "w");
        if (fp == NULL)
            perror("Opening output file failed");
        else
        {
            gdImagePng(s.img, fp);
            fclose(fp);
        }
    }

    exit(EXIT_SUCCESS);
}
