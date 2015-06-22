#include <FastLED.h>
#include <fab_utils.h>

/////////////////////// GLOBAL DEFINITIONS ////////////////////////

// LED definitions
#define LED_TYPE WS2812
#define COLOR_ORDER RGB
#define NUM_LEDS 60 
#define LED_PIN 6 

// update rate in Hz
#define UPDATE_RATE 30 

///////////////////////////////////////////////////////////////////

enum Mode
{
    IDLE = 0,
    ENERGY_PIPE = 1
};

class EnergyPipe
{
 public:

    static const int MAX_NUM_PARTICLES = 100;

    EnergyPipe(uint32_t num_leds, uint32_t led_pin):
    m_num_leds(num_leds)
    {
        m_leds = new CRGB[num_leds];
        FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(m_leds, m_num_leds);
    }
    
    ~EnergyPipe()
    {
        delete[] m_leds; 
    }

    void update(float time_delta)
    {
        // decide if new particles should be spawned
        
        float damp_factor = m_damping * time_delta;

        // update particle positions
        for(int i = 0; i < MAX_NUM_PARTICLES; i++)
        {
            Particle &p = m_particles[i];

            // skip dead particles
            if(p.life_time <= 0.f){ continue; }
            
            // update velocity 
            p.velocity += accel(p.position);
            p.velocity *= 1.f - damp_factor * time_delta; 

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
            m_leds[i] = CRGB::Blue;
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
                
                break; 
            }
        }
    }

 private:
    
    // use e.g. a parabolic formula here
    inline float accel(const float position) const 
    {
      return 0.f; 
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
        life_time(1.f)
        {}
    };
     
    // damp factor / sec
    float m_damping;
    
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

///////////////////////////////////////////////////////////////////

void setup()
{
  // init serial
  // init SPI
}

void loop()
{
    // time measurement
    int delta_time = millis() - g_last_time_stamp;
    g_last_time_stamp = millis();
    g_time_accum += delta_time;

    if(g_time_accum >= g_update_interval)
    {
        g_time_accum = 0;
        float delta_secs = delta_time / 1000.f;
        g_pipe.update(delta_secs);
    }
}

