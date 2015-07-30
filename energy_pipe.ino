#include <FastLED.h>
#include <colorpalettes.h>
#include <fab_utils.h>

/////////////////////// GLOBAL DEFINITIONS ////////////////////////

// LED definitions
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS 240 
#define LED_PIN 6 

// update rate in Hz
#define UPDATE_RATE 120 

///////////////////////////////////////////////////////////////////

enum Mode
{
    IDLE = 0,
    ENERGY_PIPE = 1
};

class EnergyPipe
{
 public:

    static const int MAX_NUM_PARTICLES = 5;

    EnergyPipe(uint32_t num_leds, uint32_t led_pin):
    m_num_leds(num_leds),
    m_fade(.75f),
    m_damping(.5f),
    m_min_life(10.f),
    m_max_life(10.f),
    m_fade_out_life(2.f)
    {
        m_leds = new CRGB[num_leds]();
        FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(m_leds, m_num_leds);
    }
    
    ~EnergyPipe()
    {
        delete[] m_leds; 
    }

    void update(float time_delta)
    {
        //TODO: decide if new particles should be spawned here !?
        
        float damp_factor = 1.f - m_damping * time_delta;

        // update particle values
        for(int i = 0; i < MAX_NUM_PARTICLES; i++)
        {
            Particle &p = m_particles[i];

            // skip dead particles
            if(p.life_time <= 0.f){ continue; }
            
            // update velocity 
            p.velocity += accel(p.position) * time_delta;
            p.velocity *= damp_factor; 

            // update position
            p.last_position = p.position;
            p.position += p.velocity * time_delta;

            // update life time
            p.life_time -= time_delta;

            // remove particles already beyond limits
            if(p.position > 2.f){ p.life_time = 0.f; }

            p.fade_out_factor = clamp<float>(p.life_time, 0.f, m_fade_out_life) / m_fade_out_life;

            // determine range of movement 
            //int index_from, index_to;
            //index_from = p.last_position * (m_num_leds - 1);
            //index_to = p.position * (m_num_leds - 1);

            //// swap indices
            //if(index_from > index_to)
            //{
            //  int tmp = index_from;
            //  index_from = index_to;
            //  index_to = tmp;
            //}
        }

        // update LED array
        for(int i = 0; i < m_num_leds; i++)
        {
            float led_pos = i / (float)(m_num_leds - 1);

            // set interpolated colour value
            for(int j = 0; j < MAX_NUM_PARTICLES; j++)
            {
                Particle &p = m_particles[j];

                // skip dead particles
                if(p.life_time <= 0.f){ continue; }
                
                // distance falloff
                float distance = p.position - led_pos;
                const float thresh = .01f;
                float distance_factor = abs(distance) > thresh ? 0 : abs(distance) / thresh;

                CRGB color = p.color;
                color.r *= distance_factor * p.fade_out_factor;
                color.g *= distance_factor * p.fade_out_factor;
                color.b *= distance_factor * p.fade_out_factor;
                m_leds[i] += color;
            }

            // fade all LEDS down
            m_leds[i].r *= m_fade;
            m_leds[i].g *= m_fade;
            m_leds[i].b *= m_fade;
        }
        FastLED.show();
    }

    void emit_particle(const CRGB &the_color, const float the_velocity)
    {
        // find the first dead particle and relive it
        for(int i = 0; i < MAX_NUM_PARTICLES; i++)
        {
            Particle &p = m_particles[i];

            if(p.life_time <= 0.f)
            { 
                p.life_time = random<float>(m_min_life, m_max_life); 
                p.last_position = p.position = 0.f;
                p.velocity = the_velocity;
                p.color = the_color;
                break; 
            }
        }
    }
    
    void set_life_time(const float the_min, const float the_max)
    {
        m_min_life = the_min;
        m_max_life = the_max;
    }

    void set_fade_out_life(const float the_life){ m_fade_out_life = the_life; }

    void set_damping(float the_damping){ m_damping = the_damping; }

    void set_fade(float the_fade){ m_fade = clamp<float>(the_fade, 0.f, 1.f); }

 private:
    
    inline float accel(const float position) const 
    {
      // f(x) = ax^4 + bx^2, f'(x) = 4ax^3 + 2bx
      const float a = -1.f, b = 2.f;
      float x_1 = 2 * (position - 0.5f);
      float ret = 4.f * a * x_1 * x_1 * x_1 + 2 * b * x_1;
      return -ret;  
    }

    struct Particle
    {
        CRGB color;
        float last_position;
        float position;
        float velocity;
        float life_time;
        float fade_out_factor;

        Particle(): 
        position(0.f),
        velocity(0.f),
        life_time(1.f),
        fade_out_factor(1.f),
        color(CRGB::Red)
        {}
    };
     
    // damp factor / sec
    float m_damping;

    // fade factor / sec
    float m_fade;
    
    // min / max life time of particles
    float m_min_life, m_max_life;

    // max lifetime without fade-out applied
    float m_fade_out_life;

    // particle objects
    Particle m_particles[MAX_NUM_PARTICLES];

    // internal LED array
    CRGB* m_leds;
    uint32_t m_num_leds;
};

/////////////////////// GLOBAL VARIABLES //////////////////////////

// current animation mode
Mode g_current_mode = ENERGY_PIPE;

// an energy pipe object 
EnergyPipe g_pipe = EnergyPipe(NUM_LEDS, LED_PIN);

// helper variables for time measurement
long g_last_time_stamp = 0;
uint32_t g_time_accum = 0;

// update interval in millis
const int g_update_interval = 1000 / UPDATE_RATE;

// time in millis until next emit
int g_emit_counter = 0;

bool g_indicator = false;

///////////////////////////////////////////////////////////////////

void setup()
{
  // init serial
  // init SPI

  // pipe settings
  g_pipe.set_life_time(12.f, 20.f);
  g_pipe.set_fade_out_life(5.f);
  g_pipe.set_damping(.6f);
}

///////////////////////////////////////////////////////////////////

void loop()
{
    // time measurement
    uint32_t delta_time = millis() - g_last_time_stamp;
    g_last_time_stamp = millis();
    g_time_accum += delta_time;
    g_emit_counter -= delta_time;

    // emit particle
    if(g_emit_counter <= 0)
    {
        g_emit_counter = random<int>(1000, 3000);
        float p_vel = random<float>(.2f, .8f);
        CRGB color = g_indicator ? CRGB::Orange : CRGB::Blue;
        //CRGB color = CRGB(random<int>(0, 255),
        //                  random<int>(0, 255),
        //                  random<int>(100, 255));

        g_pipe.emit_particle(color, p_vel);
    }

    if(g_time_accum >= g_update_interval)
    {
        float delta_secs = g_time_accum / 1000.f;
        g_time_accum = 0;
        g_pipe.update(delta_secs);

        digitalWrite(13, g_indicator);
        g_indicator = !g_indicator;
    }
}

