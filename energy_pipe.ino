#include <FastLED.h>
#include <fab_utils.h>

/////////////////////// GLOBAL DEFINITIONS ////////////////////////

// LED definitions
#define LED_TYPE WS2812
#define COLOR_ORDER GRB
#define NUM_LEDS 60 
#define LED_PIN 6 

// update rate in Hz
#define UPDATE_RATE 60 

///////////////////////////////////////////////////////////////////

enum Mode
{
    IDLE = 0,
    ENERGY_PIPE = 1
};

class EnergyPipe
{
 public:

    static const int MAX_NUM_PARTICLES = 25;

    EnergyPipe(uint32_t num_leds, uint32_t led_pin):
    m_num_leds(num_leds),
    m_fade(.75f),
    m_damping(.5f),
    m_min_life(10.f),
    m_max_life(10.f)
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
        // decide if new particles should be spawned
        
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
            p.position += p.velocity * time_delta;

            // update colour
            //p.colour = 

            // update life time
            p.life_time -= time_delta;
        }

        // update LED array
        for(int i = 0; i < m_num_leds; i++)
        {
            // set interpolated colour value
            for(int j = 0; j < MAX_NUM_PARTICLES; j++)
            {
                Particle &p = m_particles[j];

                // skip dead particles
                if(p.life_time <= 0.f){ continue; }
                
                // distance falloff
                float distance_factor = clamp<float>(abs(p.position - (i / (float)(m_num_leds - 1))), 0.f, 1.f);
                float thresh = .025f; 
                distance_factor = distance_factor > thresh ? 0 : distance_factor;

                float tmp = distance_factor / abs(distance_factor - thresh);
                CRGB color = p.color;
                color.r *= tmp;
                color.g *= tmp;
                color.b *= tmp;
                m_leds[i] += color;
            }

            // fade all LEDS down
            float fade = m_fade;
            m_leds[i].r *= fade;
            m_leds[i].g *= fade;
            m_leds[i].b *= fade;
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
                p.position = - .25f;
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

 private:
    
    // use e.g. a parabolic formula here
    inline float accel(const float position) const 
    {
      // x^2
      return 1.f * (1.f - 2.f * position); 
    }

    struct Particle
    {
        CRGB color;
        float position;
        float velocity;
        float life_time;

        Particle(): 
        position(0.f),
        velocity(0.f),
        life_time(1.f),
        color(CRGB::Red)
        {}
    };
     
    // damp factor / sec
    float m_damping;

    // fade factor / sec
    float m_fade;
    
    // min / max life time of particles
    float m_min_life, m_max_life;

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
        g_emit_counter = random<int>(5000, 10000);
        float p_vel = random<float>(0.f, .06f);
        CRGB color = CRGB(random<int>(0, 255),
                          random<int>(0, 255),
                          random<int>(100, 255));

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

