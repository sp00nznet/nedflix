/**
 * Themed Channels Configuration for ErsatzTV
 *
 * This module defines themed 24/7 channels with smart scheduling
 * that automatically populate from your media library.
 *
 * Channels use ErsatzTV's schedule blocks to create time-based
 * programming similar to traditional TV networks.
 */

// Days of week for scheduling
const DAYS = {
    SUNDAY: 0,
    MONDAY: 1,
    TUESDAY: 2,
    WEDNESDAY: 3,
    THURSDAY: 4,
    FRIDAY: 5,
    SATURDAY: 6
};

// Content rating categories (maps to common rating systems)
const RATINGS = {
    KIDS: ['G', 'TV-G', 'TV-Y', 'TV-Y7'],
    FAMILY: ['G', 'PG', 'TV-G', 'TV-PG', 'TV-Y7'],
    TEEN: ['PG', 'PG-13', 'TV-PG', 'TV-14'],
    MATURE: ['R', 'TV-MA', 'NC-17', 'NR', 'Unrated'],
    ALL: [] // No filter
};

// Genre mappings for smart collections
const GENRES = {
    ACTION: ['Action', 'Adventure', 'Thriller'],
    COMEDY: ['Comedy', 'Romantic Comedy'],
    DRAMA: ['Drama', 'Romance'],
    HORROR: ['Horror', 'Thriller', 'Slasher'],
    SCIFI: ['Science Fiction', 'Sci-Fi', 'Fantasy'],
    DOCUMENTARY: ['Documentary', 'Docuseries'],
    ANIMATION: ['Animation', 'Animated', 'Anime'],
    CLASSIC: ['Classic', 'Film-Noir', 'Western'],
    KIDS: ['Children', 'Family', 'Animation'],
    CULT: ['Cult', 'B-Movie', 'Exploitation', 'Grindhouse'],
    FOREIGN: ['Foreign', 'International']
};

/**
 * THEMED CHANNEL DEFINITIONS
 *
 * Each channel has:
 * - name: Display name
 * - number: Channel number
 * - description: What the channel shows
 * - icon: Icon identifier
 * - schedule: Array of time blocks defining what plays when
 * - collections: Smart collection queries for content
 */
const THEMED_CHANNELS = {

    // ==========================================
    // FRIDAY NIGHT MOVIE
    // ==========================================
    fridayNightMovie: {
        name: 'Friday Night Movie',
        number: 10,
        description: 'A curated movie experience every Friday at 7 PM. Classic movie night tradition.',
        icon: 'movie-night',
        schedule: [
            // Friday 7 PM - Feature Film (runs ~2-3 hours)
            {
                day: DAYS.FRIDAY,
                startTime: '19:00',
                duration: 180, // 3 hours
                collection: 'friday-features',
                playoutMode: 'shuffle',
                priority: 1
            },
            // Friday 10 PM - Late Night Double Feature
            {
                day: DAYS.FRIDAY,
                startTime: '22:00',
                duration: 240, // 4 hours
                collection: 'late-night-movies',
                playoutMode: 'shuffle',
                priority: 1
            },
            // Rest of the week - Movie previews and classics
            {
                day: 'weekdays',
                startTime: '00:00',
                duration: 1440, // All day
                collection: 'movie-trailers-classics',
                playoutMode: 'shuffle',
                priority: 0
            }
        ],
        collections: {
            'friday-features': {
                query: 'type:movie AND rating:(PG OR PG-13 OR R) AND year:>1990',
                description: 'Modern movies suitable for movie night'
            },
            'late-night-movies': {
                query: 'type:movie AND (genre:Thriller OR genre:Horror OR genre:Mystery)',
                description: 'Late night thrillers and suspense'
            },
            'movie-trailers-classics': {
                query: 'type:movie AND year:<1990',
                description: 'Classic films for filler content'
            }
        }
    },

    // ==========================================
    // INSOMNIA TV
    // ==========================================
    insomniaTV: {
        name: 'INSOMNIA TV',
        number: 11,
        description: 'Normal programming by day, edgier content after midnight. For the night owls.',
        icon: 'moon',
        schedule: [
            // 6 AM - 12 PM: Morning content (family friendly)
            {
                timeRange: { start: '06:00', end: '12:00' },
                collection: 'morning-shows',
                playoutMode: 'shuffle',
                ratings: RATINGS.FAMILY,
                description: 'Family-friendly morning programming'
            },
            // 12 PM - 6 PM: Afternoon content (general audience)
            {
                timeRange: { start: '12:00', end: '18:00' },
                collection: 'afternoon-mix',
                playoutMode: 'shuffle',
                ratings: RATINGS.TEEN,
                description: 'Afternoon variety programming'
            },
            // 6 PM - 10 PM: Prime time (teen/adult)
            {
                timeRange: { start: '18:00', end: '22:00' },
                collection: 'primetime',
                playoutMode: 'shuffle',
                ratings: RATINGS.TEEN,
                description: 'Prime time entertainment'
            },
            // 10 PM - 12 AM: Late night (mature themes)
            {
                timeRange: { start: '22:00', end: '00:00' },
                collection: 'late-night',
                playoutMode: 'shuffle',
                ratings: RATINGS.MATURE,
                description: 'Late night mature content'
            },
            // 12 AM - 6 AM: Insomnia hours (edgy/cult)
            {
                timeRange: { start: '00:00', end: '06:00' },
                collection: 'insomnia-hours',
                playoutMode: 'shuffle',
                ratings: RATINGS.MATURE,
                description: 'Late night cult classics and edgy content'
            }
        ],
        collections: {
            'morning-shows': {
                query: 'rating:(G OR PG OR TV-G OR TV-PG) AND (type:show OR type:movie)',
                description: 'Family-friendly content'
            },
            'afternoon-mix': {
                query: 'rating:(PG OR PG-13 OR TV-PG OR TV-14)',
                description: 'General audience content'
            },
            'primetime': {
                query: 'rating:(PG-13 OR TV-14 OR R) AND NOT genre:Horror',
                description: 'Prime time entertainment'
            },
            'late-night': {
                query: 'rating:(R OR TV-MA) AND (genre:Comedy OR genre:Drama)',
                description: 'Mature comedy and drama'
            },
            'insomnia-hours': {
                query: 'rating:(R OR TV-MA OR NR) AND (genre:Horror OR genre:Cult OR genre:Thriller OR genre:Foreign)',
                description: 'Cult classics, horror, and international cinema'
            }
        }
    },

    // ==========================================
    // SATURDAY MORNING CARTOONS
    // ==========================================
    saturdayMorningCartoons: {
        name: 'Saturday Morning Cartoons',
        number: 12,
        description: 'Relive the magic of Saturday morning animation. Cartoons all weekend!',
        icon: 'animation',
        schedule: [
            // Saturday 6 AM - 12 PM: Classic Saturday Morning block
            {
                day: DAYS.SATURDAY,
                startTime: '06:00',
                duration: 360, // 6 hours
                collection: 'saturday-cartoons',
                playoutMode: 'shuffle',
                priority: 1
            },
            // Sunday 7 AM - 11 AM: Sunday Funnies
            {
                day: DAYS.SUNDAY,
                startTime: '07:00',
                duration: 240,
                collection: 'sunday-animation',
                playoutMode: 'shuffle',
                priority: 1
            },
            // Weekdays: Mixed animation
            {
                day: 'weekdays',
                startTime: '00:00',
                duration: 1440,
                collection: 'all-animation',
                playoutMode: 'shuffle',
                priority: 0
            }
        ],
        collections: {
            'saturday-cartoons': {
                query: 'type:show AND genre:Animation AND rating:(TV-Y OR TV-Y7 OR TV-G OR TV-PG)',
                description: 'Kid-friendly animated shows'
            },
            'sunday-animation': {
                query: 'type:movie AND genre:Animation AND rating:(G OR PG)',
                description: 'Animated movies'
            },
            'all-animation': {
                query: 'genre:Animation',
                description: 'All animated content'
            }
        }
    },

    // ==========================================
    // MIDNIGHT MOVIES
    // ==========================================
    midnightMovies: {
        name: 'Midnight Movies',
        number: 13,
        description: 'Cult classics, B-movies, and underground cinema. The weird and wonderful.',
        icon: 'skull',
        schedule: [
            // 12 AM - 6 AM: Prime midnight viewing
            {
                timeRange: { start: '00:00', end: '06:00' },
                collection: 'midnight-cult',
                playoutMode: 'shuffle',
                priority: 1
            },
            // 6 AM - 12 AM: B-movies and exploitation
            {
                timeRange: { start: '06:00', end: '00:00' },
                collection: 'b-movies',
                playoutMode: 'shuffle',
                priority: 0
            }
        ],
        collections: {
            'midnight-cult': {
                query: 'type:movie AND (genre:Cult OR genre:Horror OR genre:Exploitation OR tag:midnight)',
                description: 'Cult classics and midnight movie fare'
            },
            'b-movies': {
                query: 'type:movie AND (genre:B-Movie OR genre:Sci-Fi OR year:<1980)',
                description: 'Classic B-movies and vintage sci-fi'
            }
        }
    },

    // ==========================================
    // BACKGROUND BEATS
    // ==========================================
    backgroundBeats: {
        name: 'Background Beats',
        number: 14,
        description: 'Music videos and concert footage. Perfect ambient entertainment.',
        icon: 'music',
        schedule: [
            // All day music rotation
            {
                timeRange: { start: '00:00', end: '24:00' },
                collection: 'music-videos',
                playoutMode: 'shuffle'
            }
        ],
        collections: {
            'music-videos': {
                query: 'type:musicvideo OR (type:movie AND genre:Music)',
                description: 'Music videos and concert films'
            }
        }
    },

    // ==========================================
    // COMEDY CENTRAL (Not affiliated!)
    // ==========================================
    comedyHour: {
        name: 'Laugh Track',
        number: 15,
        description: '24/7 comedy. Stand-up specials, comedy films, and sitcoms.',
        icon: 'comedy',
        schedule: [
            // Daytime: Sitcoms and light comedy
            {
                timeRange: { start: '06:00', end: '20:00' },
                collection: 'daytime-comedy',
                playoutMode: 'shuffle',
                ratings: RATINGS.TEEN
            },
            // Evening/Night: Stand-up and R-rated comedy
            {
                timeRange: { start: '20:00', end: '06:00' },
                collection: 'nighttime-comedy',
                playoutMode: 'shuffle',
                ratings: RATINGS.MATURE
            }
        ],
        collections: {
            'daytime-comedy': {
                query: 'genre:Comedy AND rating:(G OR PG OR PG-13 OR TV-PG OR TV-14)',
                description: 'Family and teen comedy'
            },
            'nighttime-comedy': {
                query: 'genre:Comedy AND rating:(R OR TV-MA)',
                description: 'Adult comedy and stand-up'
            }
        }
    },

    // ==========================================
    // ACTION ZONE
    // ==========================================
    actionZone: {
        name: 'Action Zone',
        number: 16,
        description: 'Non-stop action, adventure, and thrills. Explosions guaranteed.',
        icon: 'action',
        schedule: [
            // All day action
            {
                timeRange: { start: '00:00', end: '24:00' },
                collection: 'action-movies',
                playoutMode: 'shuffle'
            }
        ],
        collections: {
            'action-movies': {
                query: 'type:movie AND (genre:Action OR genre:Adventure OR genre:Thriller)',
                description: 'Action and adventure films'
            }
        }
    },

    // ==========================================
    // KIDS ZONE
    // ==========================================
    kidsZone: {
        name: 'Kids Zone',
        number: 17,
        description: 'Safe, age-appropriate content for children. Parent approved.',
        icon: 'kids',
        schedule: [
            // All day kids content with stricter ratings
            {
                timeRange: { start: '00:00', end: '24:00' },
                collection: 'kids-content',
                playoutMode: 'shuffle',
                ratings: RATINGS.KIDS
            }
        ],
        collections: {
            'kids-content': {
                query: '(genre:Children OR genre:Family OR genre:Animation) AND rating:(G OR TV-G OR TV-Y OR TV-Y7)',
                description: 'Age-appropriate children\'s content'
            }
        }
    },

    // ==========================================
    // DOCUMENTARY DISCOVERY
    // ==========================================
    documentaryDiscovery: {
        name: 'Documentary Discovery',
        number: 18,
        description: 'Explore the world through documentaries. Learn something new.',
        icon: 'documentary',
        schedule: [
            // All day documentaries
            {
                timeRange: { start: '00:00', end: '24:00' },
                collection: 'documentaries',
                playoutMode: 'shuffle'
            }
        ],
        collections: {
            'documentaries': {
                query: 'genre:Documentary',
                description: 'Documentary films and series'
            }
        }
    },

    // ==========================================
    // SCI-FI SUNDAYS
    // ==========================================
    sciFiSundays: {
        name: 'Sci-Fi Sundays',
        number: 19,
        description: 'Science fiction marathon every Sunday. Aliens, robots, and the future.',
        icon: 'scifi',
        schedule: [
            // Sunday all day: Sci-Fi marathon
            {
                day: DAYS.SUNDAY,
                startTime: '00:00',
                duration: 1440,
                collection: 'scifi-marathon',
                playoutMode: 'shuffle',
                priority: 1
            },
            // Rest of week: Mixed sci-fi
            {
                day: 'weekdays',
                startTime: '00:00',
                duration: 1440,
                collection: 'scifi-mix',
                playoutMode: 'shuffle',
                priority: 0
            }
        ],
        collections: {
            'scifi-marathon': {
                query: 'type:movie AND (genre:Science-Fiction OR genre:Sci-Fi)',
                description: 'Science fiction movies'
            },
            'scifi-mix': {
                query: 'genre:Science-Fiction OR genre:Sci-Fi OR genre:Fantasy',
                description: 'Sci-fi and fantasy mix'
            }
        }
    },

    // ==========================================
    // THROWBACK THURSDAY
    // ==========================================
    throwbackThursday: {
        name: 'Throwback Thursday',
        number: 20,
        description: 'Classics from decades past. Every Thursday is a trip through time.',
        icon: 'retro',
        schedule: [
            // Thursday: Decade blocks
            {
                day: DAYS.THURSDAY,
                startTime: '06:00',
                duration: 360, // 6 AM - 12 PM
                collection: 'classics-50s-60s',
                playoutMode: 'chronological',
                priority: 1
            },
            {
                day: DAYS.THURSDAY,
                startTime: '12:00',
                duration: 360, // 12 PM - 6 PM
                collection: 'classics-70s-80s',
                playoutMode: 'chronological',
                priority: 1
            },
            {
                day: DAYS.THURSDAY,
                startTime: '18:00',
                duration: 360, // 6 PM - 12 AM
                collection: 'classics-90s',
                playoutMode: 'chronological',
                priority: 1
            },
            // Other days: Mixed classics
            {
                day: 'other',
                startTime: '00:00',
                duration: 1440,
                collection: 'all-classics',
                playoutMode: 'shuffle',
                priority: 0
            }
        ],
        collections: {
            'classics-50s-60s': {
                query: 'type:movie AND year:>=1950 AND year:<=1969',
                description: '1950s and 1960s classics'
            },
            'classics-70s-80s': {
                query: 'type:movie AND year:>=1970 AND year:<=1989',
                description: '1970s and 1980s classics'
            },
            'classics-90s': {
                query: 'type:movie AND year:>=1990 AND year:<=1999',
                description: '1990s classics'
            },
            'all-classics': {
                query: 'type:movie AND year:<2000',
                description: 'All pre-2000 content'
            }
        }
    },

    // ==========================================
    // DRAMA QUEENS
    // ==========================================
    dramaQueens: {
        name: 'Drama Queens',
        number: 21,
        description: 'Emotional stories, intense performances, award-winning drama.',
        icon: 'drama',
        schedule: [
            {
                timeRange: { start: '00:00', end: '24:00' },
                collection: 'drama',
                playoutMode: 'shuffle'
            }
        ],
        collections: {
            'drama': {
                query: 'type:movie AND (genre:Drama OR genre:Romance)',
                description: 'Drama and romance films'
            }
        }
    },

    // ==========================================
    // HORROR HOUSE
    // ==========================================
    horrorHouse: {
        name: 'Horror House',
        number: 22,
        description: 'Scares around the clock. Horror movies, thrillers, and the macabre.',
        icon: 'horror',
        schedule: [
            // Daytime: Lighter horror/thriller
            {
                timeRange: { start: '06:00', end: '20:00' },
                collection: 'light-horror',
                playoutMode: 'shuffle'
            },
            // Nighttime: Intense horror
            {
                timeRange: { start: '20:00', end: '06:00' },
                collection: 'intense-horror',
                playoutMode: 'shuffle'
            }
        ],
        collections: {
            'light-horror': {
                query: 'type:movie AND genre:Horror AND rating:(PG-13 OR TV-14)',
                description: 'Lighter horror and suspense'
            },
            'intense-horror': {
                query: 'type:movie AND genre:Horror AND rating:(R OR TV-MA OR NR)',
                description: 'Intense and graphic horror'
            }
        }
    },

    // ==========================================
    // WORLD CINEMA
    // ==========================================
    worldCinema: {
        name: 'World Cinema',
        number: 23,
        description: 'International films from around the globe. Subtitles recommended.',
        icon: 'globe',
        schedule: [
            {
                timeRange: { start: '00:00', end: '24:00' },
                collection: 'international',
                playoutMode: 'shuffle'
            }
        ],
        collections: {
            'international': {
                query: 'type:movie AND (genre:Foreign OR language:!English)',
                description: 'Non-English language films'
            }
        }
    },

    // ==========================================
    // BINGE WATCH (TV Marathon)
    // ==========================================
    bingeWatch: {
        name: 'Binge Watch',
        number: 24,
        description: 'TV show marathons. Full seasons back to back.',
        icon: 'tv',
        schedule: [
            {
                timeRange: { start: '00:00', end: '24:00' },
                collection: 'tv-shows',
                playoutMode: 'chronological' // Play episodes in order
            }
        ],
        collections: {
            'tv-shows': {
                query: 'type:show',
                description: 'All TV shows'
            }
        }
    },

    // ==========================================
    // AMBIENT CINEMA
    // ==========================================
    ambientCinema: {
        name: 'Ambient Cinema',
        number: 25,
        description: 'Slow cinema, nature docs, and relaxing visuals. Perfect for winding down.',
        icon: 'zen',
        schedule: [
            {
                timeRange: { start: '00:00', end: '24:00' },
                collection: 'ambient',
                playoutMode: 'shuffle'
            }
        ],
        collections: {
            'ambient': {
                query: '(genre:Documentary AND (tag:nature OR tag:ambient)) OR (type:movie AND genre:Art-House)',
                description: 'Relaxing and ambient content'
            }
        }
    }
};

/**
 * Get all themed channel configurations
 */
function getAllChannels() {
    return Object.values(THEMED_CHANNELS);
}

/**
 * Get a specific channel by key
 */
function getChannel(key) {
    return THEMED_CHANNELS[key];
}

/**
 * Get channels by category/type
 */
function getChannelsByType(type) {
    const typeMap = {
        'movies': ['fridayNightMovie', 'midnightMovies', 'actionZone', 'dramaQueens', 'worldCinema'],
        'tv': ['bingeWatch', 'saturdayMorningCartoons'],
        'horror': ['horrorHouse', 'midnightMovies'],
        'kids': ['kidsZone', 'saturdayMorningCartoons'],
        'music': ['backgroundBeats'],
        'documentary': ['documentaryDiscovery', 'ambientCinema'],
        'scifi': ['sciFiSundays'],
        'comedy': ['comedyHour'],
        'scheduled': ['fridayNightMovie', 'saturdayMorningCartoons', 'throwbackThursday', 'sciFiSundays'],
        'dayparted': ['insomniaTV', 'horrorHouse', 'comedyHour']
    };

    return (typeMap[type] || []).map(key => THEMED_CHANNELS[key]).filter(Boolean);
}

/**
 * Convert schedule to ErsatzTV schedule block format
 */
function toErsatzTVSchedule(channel) {
    const blocks = [];

    for (const sched of channel.schedule) {
        const block = {
            playoutMode: sched.playoutMode || 'Shuffle',
            collectionQuery: channel.collections[sched.collection]?.query || '',
            collectionName: sched.collection
        };

        // Handle day-specific schedules
        if (sched.day !== undefined) {
            if (sched.day === 'weekdays') {
                block.days = [1, 2, 3, 4, 5]; // Mon-Fri
            } else if (sched.day === 'other') {
                block.days = [0, 1, 2, 3, 5, 6]; // All except Thursday
            } else {
                block.days = [sched.day];
            }
            block.startTime = sched.startTime;
            block.durationMinutes = sched.duration;
        }

        // Handle time range schedules (daily)
        if (sched.timeRange) {
            block.startTime = sched.timeRange.start;
            block.endTime = sched.timeRange.end;
            block.days = [0, 1, 2, 3, 4, 5, 6]; // All days
        }

        if (sched.ratings) {
            block.ratingFilter = sched.ratings;
        }

        block.priority = sched.priority || 0;
        blocks.push(block);
    }

    return blocks;
}

/**
 * Generate ErsatzTV API payload for creating a channel
 */
function toErsatzTVChannelPayload(channel) {
    return {
        name: channel.name,
        number: String(channel.number),
        description: channel.description,
        preferredAudioLanguageCode: 'eng',
        subtitleMode: 'None',
        streamingMode: 'TransportStream',
        // Schedule blocks would be added via separate API calls
        scheduleBlocks: toErsatzTVSchedule(channel)
    };
}

module.exports = {
    THEMED_CHANNELS,
    DAYS,
    RATINGS,
    GENRES,
    getAllChannels,
    getChannel,
    getChannelsByType,
    toErsatzTVSchedule,
    toErsatzTVChannelPayload
};
